#include "stdafx.h"
#include "Session.h"

Session::Session()
	: duration(5)
{
}

Session::~Session()
{
}

pj_status_t Session::Prepare( pjsip_endpoint *sip_endpt, 
							 pjmedia_endpt *media_endpt,
							 pj_str_t local_addr,
							 pj_uint16_t &media_port,
							 pj_uint8_t call_idx,
							 int mod_id )
{
	pj_status_t status;

	this->index = call_idx;
	this->media_endpt = media_endpt;
	this->sip_endpt = sip_endpt;
	this->mod_id = mod_id;

	/* Create transport for each media in the call */
	for ( pj_uint8_t media_idx = 0; media_idx < PJ_ARRAY_SIZE(medias); ++ media_idx )
	{
	    /* Repeat binding media socket to next port when fails to bind
	     * to current port number.
	     */
	    int retry;

	    medias[media_idx].call_index = call_idx;
	    medias[media_idx].media_index = media_idx;

	    status = -1;
	    for ( retry = 0; retry < 100; ++ retry, media_port += 2 )
		{
			struct media_stream *m = &medias[media_idx];

			status = pjmedia_transport_udp_create2(media_endpt, 
				"siprtp",
				&local_addr,
				media_port, 0, 
				&m->transport);
			if (status == PJ_SUCCESS)
			{
				media_port += 2;
				break;
			}
	    }
	}

	return status;
}

void Session::Launch()
{
}

pj_status_t Session::Start(const pj_str_t *local_uri, const pj_str_t *remote_uri, pj_int32_t module_id)
{
    pjsip_dialog *dlg;
    pjmedia_sdp_session *sdp = NULL;
    pjsip_tx_data *tdata;
    pj_status_t status;

    /* Create UAC dialog */
    status = pjsip_dlg_create_uac( pjsip_ua_instance(), 
				   local_uri,	    /* local URI	    */
				   NULL,        	/* local Contact    */
				   remote_uri,		/* remote URI	    */
				   NULL,		    /* remote target    */
				   &dlg);		    /* dialog	        */
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    /* Create SDP */
    //create_sdp( dlg->pool, call, &sdp);

    /* Create the INVITE session. */
    status = pjsip_inv_create_uac(dlg, sdp, 0, &inv_session);
    if (status != PJ_SUCCESS)
	{
		pjsip_dlg_terminate(dlg);
		return status;
    }

    /* Attach call data to invite session */
	inv_session->mod_data[module_id] = this;

    /* Mark start of call */
    pj_gettimeofday(&start_time);

    /* Create initial INVITE request.
     * This INVITE request will contain a perfectly good request and 
     * an SDP body as well.
     */
    status = pjsip_inv_invite(inv_session, &tdata);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    /* Send initial INVITE request. 
     * From now on, the invite session's state will be reported to us
     * via the invite session callbacks.
     */
    status = pjsip_inv_send_msg(inv_session, tdata);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    return PJ_SUCCESS;
}

void Session::Stop()
{
	for ( int idx = 0; idx < ARRAYSIZE(medias); ++ idx )
	{
		struct media_stream *media = &medias[idx];
		if (media)
		{
			media->active = PJ_FALSE;

			if (media->thread)
			{
				media->thread_quit_flag = 1;
				pj_thread_join(media->thread);
				pj_thread_destroy(media->thread);
				media->thread = NULL;
				media->thread_quit_flag = 0;
			}

			pjmedia_transport_detach(media->transport, media);
		}
	}
}

void Session::Hangup()
{
	pjsip_tx_data *tdata;
    pj_status_t status;

    if (inv_session == NULL)
	{
		return;
	}

    status = pjsip_inv_end_session(app.call[index].inv, 603, NULL, &tdata);
    if (status==PJ_SUCCESS && tdata!=NULL)
	{
		pjsip_inv_send_msg(app.call[index].inv, tdata);
	}
}

void Session::OnConnecting()
{
}

void Session::OnConfirmed()
{
	pj_time_val t;

	pj_gettimeofday(&connect_time);
	if (response_time.sec == 0)
	    response_time = connect_time;

	t = connect_time;
	PJ_TIME_VAL_SUB(t, start_time);

	PJ_LOG(3,(THIS_FILE, "Call #%d connected in %d ms", index,
		  PJ_TIME_VAL_MSEC(t)));

	if (duration != 0)
	{
	    d_timer.id = 1;
	    d_timer.user_data = this;
	    d_timer.cb = &timer_disconnect_call;

	    t.sec = duration;
	    t.msec = 0;

	    pjsip_endpt_schedule_timer(sip_endpt, &d_timer, &t);
	}
}

void Session::OnDisconnected()
{
	pj_time_val null_time = {0, 0};

	if (d_timer.id != 0) {
	    pjsip_endpt_cancel_timer(sip_endpt, &d_timer);
	    d_timer.id = 0;
	}

	PJ_LOG(3,(THIS_FILE, "Call #%d disconnected. Reason=%d (%.*s)",
		  index,
		  inv_session->cause,
		  (int)inv_session->cause_text.slen,
		  inv_session->cause_text.ptr));

    PJ_LOG(3,(THIS_FILE, "Call #%d statistics:", index));
    Statistic();

	this->inv_session = NULL;
	inv_session->mod_data[this->mod_id] = NULL;

	Stop();

	start_time = null_time;
	response_time = null_time;
	connect_time = null_time;
}

const char *Session::good_number(char *buf, pj_int32_t val)
{
    if (val < 1000)
	{
		pj_ansi_sprintf(buf, "%d", val);
    }
	else if(val < 1000000)
	{
		pj_ansi_sprintf(buf, "%d.%02dK", 
				val / 1000,
				(val % 1000) / 100);
    }
	else
	{
		pj_ansi_sprintf(buf, "%d.%02dM", 
				val / 1000000,
				(val % 1000000) / 10000);
    }

    return buf;
}

void Session::Statistic()
{
    int len;
	pjsip_inv_session *inv = this->inv_session;
    pjsip_dialog *dlg = inv->dlg;
    struct media_stream *audio = &medias[0];
    char userinfo[128];
    char duration[80], last_update[80];
    char bps[16], ipbps[16], packets[16], bytes[16], ipbytes[16];
    unsigned decor;
    pj_time_val now;

    decor = pj_log_get_decor();
    pj_log_set_decor(PJ_LOG_HAS_NEWLINE);

    pj_gettimeofday(&now);

    /* Print duration */
    if (inv->state >= PJSIP_INV_STATE_CONFIRMED && connect_time.sec) {
		PJ_TIME_VAL_SUB(now, connect_time);

		sprintf(duration, " [duration: %02ld:%02ld:%02ld.%03ld]",
			now.sec / 3600,
			(now.sec % 3600) / 60,
			(now.sec % 60),
			now.msec);
    }
	else
	{
		duration[0] = '\0';
    }

    /* Call number and state */
    PJ_LOG(3, (THIS_FILE,
	      "Call #%d: %s%s", 
	      index, pjsip_inv_state_name(inv->state), 
	      duration));

    /* Call identification */
    len = pjsip_hdr_print_on(dlg->remote.info, userinfo, sizeof(userinfo));
    if (len < 0)
	{
		pj_ansi_strcpy(userinfo, "<--uri too long-->");
	}
    else
	{
		userinfo[len] = '\0';
	}

    PJ_LOG(3, (THIS_FILE, "   %s", userinfo));

	if (inv_session == NULL ||
		inv_session->state < PJSIP_INV_STATE_CONFIRMED ||
		connect_time.sec == 0) 
    {
		pj_log_set_decor(decor);
		return;
    }

    /* Signaling quality */
    {
		char pdd[64], connectdelay[64];
		pj_time_val t;

		if (response_time.sec)
		{
			t = response_time;
			PJ_TIME_VAL_SUB(t, start_time);
			sprintf(pdd, "got 1st response in %ld ms", PJ_TIME_VAL_MSEC(t));
		}
		else
		{
			pdd[0] = '\0';
		}

		if (connect_time.sec)
		{
			t = connect_time;
			PJ_TIME_VAL_SUB(t, start_time);
			sprintf(connectdelay, ", connected after: %ld ms", PJ_TIME_VAL_MSEC(t));
		}
		else
		{
			connectdelay[0] = '\0';
		}

		PJ_LOG(3, (THIS_FILE, "   Signaling quality: %s%s", pdd, connectdelay));
    }


    PJ_LOG(3, (THIS_FILE, "   Stream #0: audio %.*s@%dHz, %dms/frame, %sB/s (%sB/s +IP hdr)",
		(int)audio->ai.fmt.encoding_name.slen,
		audio->ai.fmt.encoding_name.ptr,
		audio->clock_rate,
		audio->samples_per_frame * 1000 / audio->clock_rate,
		good_number(bps, audio->bytes_per_frame * audio->clock_rate / audio->samples_per_frame),
		good_number(ipbps, (audio->bytes_per_frame+32) * audio->clock_rate / audio->samples_per_frame)));

    if (audio->rtcp.stat.rx.update_cnt == 0)
	strcpy(last_update, "never");
    else {
	pj_gettimeofday(&now);
	PJ_TIME_VAL_SUB(now, audio->rtcp.stat.rx.update);
	sprintf(last_update, "%02ldh:%02ldm:%02ld.%03lds ago",
		now.sec / 3600,
		(now.sec % 3600) / 60,
		now.sec % 60,
		now.msec);
    }

    PJ_LOG(3, (THIS_FILE, 
	   "              RX stat last update: %s\n"
	   "                 total %s packets %sB received (%sB +IP hdr)%s\n"
	   "                 pkt loss=%d (%3.1f%%), dup=%d (%3.1f%%), reorder=%d (%3.1f%%)%s\n"
	   "                       (msec)    min     avg     max     last\n"
	   "                 loss period: %7.3f %7.3f %7.3f %7.3f%s\n"
	   "                 jitter     : %7.3f %7.3f %7.3f %7.3f%s",
	   last_update,
	   good_number(packets, audio->rtcp.stat.rx.pkt),
	   good_number(bytes, audio->rtcp.stat.rx.bytes),
	   good_number(ipbytes, audio->rtcp.stat.rx.bytes + audio->rtcp.stat.rx.pkt * 32),
	   "",
	   audio->rtcp.stat.rx.loss,
	   audio->rtcp.stat.rx.loss * 100.0 / (audio->rtcp.stat.rx.pkt + audio->rtcp.stat.rx.loss),
	   audio->rtcp.stat.rx.dup, 
	   audio->rtcp.stat.rx.dup * 100.0 / (audio->rtcp.stat.rx.pkt + audio->rtcp.stat.rx.loss),
	   audio->rtcp.stat.rx.reorder, 
	   audio->rtcp.stat.rx.reorder * 100.0 / (audio->rtcp.stat.rx.pkt + audio->rtcp.stat.rx.loss),
	   "",
	   audio->rtcp.stat.rx.loss_period.min / 1000.0, 
	   audio->rtcp.stat.rx.loss_period.mean / 1000.0, 
	   audio->rtcp.stat.rx.loss_period.max / 1000.0,
	   audio->rtcp.stat.rx.loss_period.last / 1000.0,
	   "",
	   audio->rtcp.stat.rx.jitter.min / 1000.0,
	   audio->rtcp.stat.rx.jitter.mean / 1000.0,
	   audio->rtcp.stat.rx.jitter.max / 1000.0,
	   audio->rtcp.stat.rx.jitter.last / 1000.0,
	   ""
	   ));


    if (audio->rtcp.stat.tx.update_cnt == 0)
	strcpy(last_update, "never");
    else {
	pj_gettimeofday(&now);
	PJ_TIME_VAL_SUB(now, audio->rtcp.stat.tx.update);
	sprintf(last_update, "%02ldh:%02ldm:%02ld.%03lds ago",
		now.sec / 3600,
		(now.sec % 3600) / 60,
		now.sec % 60,
		now.msec);
    }

    PJ_LOG(3, (THIS_FILE,
	   "              TX stat last update: %s\n"
	   "                 total %s packets %sB sent (%sB +IP hdr)%s\n"
	   "                 pkt loss=%d (%3.1f%%), dup=%d (%3.1f%%), reorder=%d (%3.1f%%)%s\n"
	   "                       (msec)    min     avg     max     last\n"
	   "                 loss period: %7.3f %7.3f %7.3f %7.3f%s\n"
	   "                 jitter     : %7.3f %7.3f %7.3f %7.3f%s",
	   last_update,
	   good_number(packets, audio->rtcp.stat.tx.pkt),
	   good_number(bytes, audio->rtcp.stat.tx.bytes),
	   good_number(ipbytes, audio->rtcp.stat.tx.bytes + audio->rtcp.stat.tx.pkt * 32),
	   "",
	   audio->rtcp.stat.tx.loss,
	   audio->rtcp.stat.tx.loss * 100.0 / (audio->rtcp.stat.tx.pkt + audio->rtcp.stat.tx.loss),
	   audio->rtcp.stat.tx.dup, 
	   audio->rtcp.stat.tx.dup * 100.0 / (audio->rtcp.stat.tx.pkt + audio->rtcp.stat.tx.loss),
	   audio->rtcp.stat.tx.reorder, 
	   audio->rtcp.stat.tx.reorder * 100.0 / (audio->rtcp.stat.tx.pkt + audio->rtcp.stat.tx.loss),
	   "",
	   audio->rtcp.stat.tx.loss_period.min / 1000.0, 
	   audio->rtcp.stat.tx.loss_period.mean / 1000.0, 
	   audio->rtcp.stat.tx.loss_period.max / 1000.0,
	   audio->rtcp.stat.tx.loss_period.last / 1000.0,
	   "",
	   audio->rtcp.stat.tx.jitter.min / 1000.0,
	   audio->rtcp.stat.tx.jitter.mean / 1000.0,
	   audio->rtcp.stat.tx.jitter.max / 1000.0,
	   audio->rtcp.stat.tx.jitter.last / 1000.0,
	   ""
	   ));


    PJ_LOG(3, (THIS_FILE,
	   "             RTT delay      : %7.3f %7.3f %7.3f %7.3f%s\n", 
	   audio->rtcp.stat.rtt.min / 1000.0,
	   audio->rtcp.stat.rtt.mean / 1000.0,
	   audio->rtcp.stat.rtt.max / 1000.0,
	   audio->rtcp.stat.rtt.last / 1000.0,
	   ""
	   ));

    pj_log_set_decor(decor);
}