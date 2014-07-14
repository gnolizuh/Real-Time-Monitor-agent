#include "stdafx.h"
#include "Session.h"

struct codec Session::g_audio_codecs[] = 
{
    { 0,  "PCMU", 8000, 64000, 20, "G.711 ULaw" },
    { 3,  "GSM",  8000, 13200, 20, "GSM" },
    { 4,  "G723", 8000, 6400,  30, "G.723.1" },
    { 8,  "PCMA", 8000, 64000, 20, "G.711 ALaw" },
    { 18, "G729", 8000, 8000,  20, "G.729" },
};

struct codec Session::g_video_codecs[] = 
{
    { 97,  "H264", 90000, 62208000/* bits-rate */, 6000/* samples per frame */, "H.264/AVC" },
};

Session::Session(pj_uint32_t index)
	: index_(index)
	, module_id_(-1)
	, invite_session_(NULL)
	, medias_count_(2)
	, bind_addr_()
	, start_time_()
	, response_time_()
	, connect_time_()
	, media_endpt_(NULL)
	, sip_endpt_(NULL)
	, medias_array_(medias_count_)
	, msg_queue_(NULL)
{
}

Session::~Session()
{
}

pj_status_t Session::Prepare( pjsip_endpoint *sip_endpt, 
							 pjmedia_endpt *media_endpt,
							 pj_str_t bind_addr,
							 pj_uint16_t &media_port,
							 int module_id )
{
	pj_status_t status;

	set_media_endpt(media_endpt);
	set_sip_endpt(sip_endpt);
	set_module_id(module_id);
	set_bind_addr(bind_addr);
	set_msg_queue(ScreenMgr::GetInstance()->GetMessageQueue(index()));

	/* Create transport for each media in the call */
	for ( pj_uint32_t media_idx = 0; media_idx < medias_array().size(); ++ media_idx )
	{
	    /* Repeat binding media socket to next port when fails to bind
	     * to current port number.
	     */
	    int retry;

	    medias_array()[media_idx].sess_index = index();
	    medias_array()[media_idx].media_index = media_idx;

	    status = -1;
	    for ( retry = 0; retry < 100; ++ retry, media_port += 2 )
		{
			struct media_stream *m = &(medias_array()[media_idx]);

			status = pjmedia_transport_udp_create2(media_endpt, 
				"siprtp",
				&bind_addr,
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
	return;
}

pj_status_t Session::Invite(const pj_str_t *local_uri, const pj_str_t *remote_uri, pj_int32_t module_id)
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
    CreateSdp(dlg->pool, &sdp);

    /* Create the INVITE session. */
    status = pjsip_inv_create_uac(dlg, sdp, 0, &invite_session_);
    if (status != PJ_SUCCESS)
	{
		pjsip_dlg_terminate(dlg);
		return status;
    }

    /* Attach call data to invite session */
	invite_session()->mod_data[module_id] = this;

    /* Mark start of call */
    pj_gettimeofday(&start_time());

    /* Create initial INVITE request.
     * This INVITE request will contain a perfectly good request and 
     * an SDP body as well.
     */
    status = pjsip_inv_invite(invite_session(), &tdata);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    /* Send initial INVITE request. 
     * From now on, the invite session's state will be reported to us
     * via the invite session callbacks.
     */
    status = pjsip_inv_send_msg(invite_session(), tdata);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    return PJ_SUCCESS;
}

pj_status_t Session::Hangup()
{
	pjsip_tx_data *tdata;
	pj_status_t status;

    if (invite_session() == NULL)
	{
		return -1;
	}

    status = pjsip_inv_end_session(invite_session(), 603, NULL, &tdata);
    if (status==PJ_SUCCESS && tdata!=NULL)
	{
		pjsip_inv_send_msg(invite_session(), tdata);
	}

	return status;
}

pj_status_t Session::Stop()
{
	for ( pj_uint32_t idx = 0; idx < medias_array().size(); ++ idx )
	{
		struct media_stream *media = &(medias_array()[idx]);
		if (media)
		{
			media->active = PJ_FALSE;

			pjmedia_transport_detach(media->transport, media);
		}
	}

	return PJ_SUCCESS;
}

void Session::OnConnecting(pjsip_inv_session *inv, pjsip_event *e)
{
	if (response_time().sec == 0)
	{
	    pj_gettimeofday(&response_time());
	}

	util_packet_t *packet = new util_packet_t();
	packet->type = sinashow::UTIL_PACKET_SIP;
	packet->buf = new sinashow::util_sip_packet_type_t();
	packet->buflen = sizeof(sinashow::util_sip_packet_type_t);
	*((sinashow::util_sip_packet_type_t *)packet->buf) = sinashow::UTIL_SIP_PACKET_CONNECTING;

	msg_queue()->Push(packet);
}

void Session::OnConfirmed(pjsip_inv_session *inv, pjsip_event *e)
{
	pj_time_val t;

	pj_gettimeofday(&connect_time());
	if (response_time().sec == 0)
	{
	    set_response_time(connect_time());
	}

	t = connect_time();
	PJ_TIME_VAL_SUB(t, start_time());

	PJ_LOG(3,(THIS_FILE, "Call #%d connected in %d ms", index(),
		  PJ_TIME_VAL_MSEC(t)));

	util_packet_t *packet = new util_packet_t();
	packet->type = sinashow::UTIL_PACKET_SIP;
	packet->buf = new sinashow::util_sip_packet_type_t();
	packet->buflen = sizeof(sinashow::util_sip_packet_type_t);
	*((sinashow::util_sip_packet_type_t *)packet->buf) = sinashow::UTIL_SIP_PACKET_CONNECTED;

	msg_queue()->Push(packet);
}

void Session::OnDisconnected(pjsip_inv_session *inv, pjsip_event *e)
{
	pj_time_val null_time = {0, 0};

	PJ_LOG(3,(THIS_FILE, "Call #%d disconnected. Reason=%d (%.*s)",
		  index(),
		  invite_session()->cause,
		  (int)invite_session()->cause_text.slen,
		  invite_session()->cause_text.ptr));

    PJ_LOG(3,(THIS_FILE, "Call #%d statistics:", index()));
    // Statistic();

	set_invite_session(NULL);
	inv->mod_data[module_id()];

	Stop();

	set_start_time(null_time);
	set_response_time(null_time);
	set_connect_time(null_time);

	util_packet_t *packet = new util_packet_t();
	packet->type = sinashow::UTIL_PACKET_SIP;
	packet->buf = new sinashow::util_sip_packet_type_t();
	packet->buflen = sizeof(sinashow::util_sip_packet_type_t);
	*((sinashow::util_sip_packet_type_t *)packet->buf) = sinashow::UTIL_SIP_PACKET_DISCONNECTED;

	msg_queue()->Push(packet);
}

void Session::OnTimerStopSession(pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry)
{
	Session *session = static_cast<Session *>(entry->user_data);

    PJ_UNUSED_ARG(timer_heap);

    entry->id = 0;
	session->Hangup();
}

void Session::OnRxRtp(void *user_data, void *pkt, pj_ssize_t size)
{
	struct media_stream *strm;
    pj_status_t status;
    const pjmedia_rtp_hdr *hdr;
    const void *payload;
    unsigned payload_len;

    strm = (struct media_stream *)user_data;

    /* Discard packet if media is inactive */
    if (!strm->active)
	return;

    /* Check for errors */
	PJ_RETURN_IF_FALSE(size >= 0);

    /* Decode RTP packet. */
    status = pjmedia_rtp_decode_rtp(&strm->in_sess, 
				    pkt, (int)size, 
				    &hdr, &payload, &payload_len);
	PJ_RETURN_IF_FALSE(status == PJ_SUCCESS);

    //PJ_LOG(4,(THIS_FILE, "Rx seq=%d", pj_ntohs(hdr->seq)));

    /* Update the RTCP session. */
    pjmedia_rtcp_rx_rtp(&strm->rtcp, pj_ntohs(hdr->seq),
			pj_ntohl(hdr->ts), payload_len);

    /* Update RTP session */
    pjmedia_rtp_session_update(&strm->in_sess, hdr, NULL);

	util_packet_t *packet = new util_packet_t();
	packet->type = sinashow::UTIL_PACKET_MEDIA;
	packet->buf = new sinashow::util_sip_packet_type_t();
	packet->buflen = sizeof(sinashow::util_sip_packet_type_t);
	*((sinashow::util_sip_packet_type_t *)packet->buf) = sinashow::UTIL_SIP_PACKET_DISCONNECTED;

	ScreenMgr::GetInstance()->PushScreenPacket(packet, strm->sess_index);
}

void Session::OnRxRtcp(void *user_data, void *pkt, pj_ssize_t size)
{
	struct media_stream *strm;

    strm = (struct media_stream *)user_data;

    /* Discard packet if media is inactive */
    if (!strm->active)
	return;

    /* Check for errors */
    PJ_RETURN_IF_FALSE(size >= 0);

    /* Update RTCP session */
    pjmedia_rtcp_rx_rtcp(&strm->rtcp, pkt, size);
}

void Session::UpdateMedia(pjsip_inv_session *inv, pj_status_t status)
{
	pj_pool_t *pool;
    struct media_stream *audio, *video;
    const pjmedia_sdp_session *local_sdp, *remote_sdp;
    struct codec *aud_codec_desc = NULL, *vid_codec_desc = NULL;
    unsigned i;

	pool = inv->dlg->pool;
    audio = &medias_array()[0];
	video = &medias_array()[1];

    /* If this is a mid-call media update, then destroy existing media */
	audio->active = PJ_FALSE;
	pjmedia_transport_detach(audio->transport, audio);

	video->active = PJ_FALSE;
	pjmedia_transport_detach(video->transport, video);

    /* Do nothing if media negotiation has failed */
    PJ_RETURN_IF_FALSE(status == PJ_SUCCESS);
    
    /* Capture stream definition from the SDP */
    pjmedia_sdp_neg_get_active_local(inv->neg, &local_sdp);
    pjmedia_sdp_neg_get_active_remote(inv->neg, &remote_sdp);

    status = pjmedia_stream_info_from_sdp(&audio->ai, inv->pool, media_endpt(),
					  local_sdp, remote_sdp, 0);
	PJ_RETURN_IF_FALSE(status == PJ_SUCCESS);

	status = pjmedia_vid_stream_info_from_sdp(&video->vi, inv->pool, media_endpt(), local_sdp, remote_sdp, 1);
	PJ_RETURN_IF_FALSE(status == PJ_SUCCESS);

	/* Find the codec description in codec array */
	for (i=0; i<PJ_ARRAY_SIZE(g_audio_codecs); ++i)
	{
		if (g_audio_codecs[i].pt == audio->ai.fmt.pt)
		{
			aud_codec_desc = &g_audio_codecs[i];
			break;
		}
	}

	if (aud_codec_desc == NULL)
	{
		PJ_LOG(3, (THIS_FILE, "Error: Invalid audio codec payload type"));
		return;
	}

	/* Find the codec description in codec array */
	for (i=0; i<PJ_ARRAY_SIZE(g_video_codecs); ++i)
	{
		if (g_video_codecs[i].pt == video->vi.codec_info.pt) {
		vid_codec_desc = &g_video_codecs[i];
		break;
		}
	}

	if (vid_codec_desc == NULL) {
		PJ_LOG(3, (THIS_FILE, "Error: Invalid video codec payload type"));
		return;
	}

    audio->clock_rate = audio->ai.fmt.clock_rate;
    audio->samples_per_frame = audio->clock_rate * aud_codec_desc->ptime / 1000;
    audio->bytes_per_frame = aud_codec_desc->bit_rate * aud_codec_desc->ptime / 1000 / 8;

	// Support video; set incoming call fps and bps
	pjmedia_ratio ifps = video->vi.codec_param->dec_fmt.det.vid.fps;
	video->clock_rate = video->vi.codec_info.clock_rate;
	video->samples_per_frame = video->clock_rate / (ifps.num / ifps.denum);
	video->bytes_per_frame = vid_codec_desc->bit_rate / (ifps.num / ifps.denum) / 8;

    pjmedia_rtp_session_init(&audio->out_sess, audio->ai.tx_pt, 
			     pj_rand());
    pjmedia_rtp_session_init(&audio->in_sess, audio->ai.fmt.pt, 0);
    pjmedia_rtcp_init(&audio->rtcp, "audio_rtcp", audio->clock_rate, 
		      audio->samples_per_frame, 0);

	pjmedia_rtp_session_init(&video->out_sess, video->vi.tx_pt, pj_rand());
	pjmedia_rtp_session_init(&video->in_sess, video->vi.rx_pt, 0);
	pjmedia_rtcp_init(&video->rtcp, "video_rtcp", video->clock_rate, video->samples_per_frame, 0);

    /* Attach media to transport */
    status = pjmedia_transport_attach(audio->transport, audio, 
				      &audio->ai.rem_addr, 
				      &audio->ai.rem_rtcp, 
				      sizeof(pj_sockaddr_in),
				      &OnRxRtp,
				      &OnRxRtcp);
	PJ_RETURN_IF_FALSE(status == PJ_SUCCESS);

	/* Attach media to transport */
	status = pjmedia_transport_attach(video->transport, video, 
						&video->vi.rem_addr, 
						&video->vi.rem_rtcp, 
						sizeof(pj_sockaddr_in),
						&OnRxRtp,
						&OnRxRtcp);
	PJ_RETURN_IF_FALSE(status == PJ_SUCCESS);

	audio->active = PJ_TRUE;
	video->active = PJ_TRUE;
}

pj_status_t Session::CreateSdp( pj_pool_t *pool, pjmedia_sdp_session **p_sdp)
{
	pj_time_val tv;
    pjmedia_sdp_session *sdp;
    pjmedia_sdp_media *m;
    pjmedia_sdp_attr *attr;
    pjmedia_transport_info tpinfo;
    struct media_stream *audio = &medias_array()[0];
	struct media_stream *video = &medias_array()[1];

    PJ_ASSERT_RETURN(pool && p_sdp, PJ_EINVAL);

    /* Get transport info */
    pjmedia_transport_info_init(&tpinfo);
    pjmedia_transport_get_info(audio->transport, &tpinfo);

    /* Create and initialize basic SDP session */
    sdp = (pjmedia_sdp_session *)pj_pool_zalloc (pool, sizeof(pjmedia_sdp_session));

    pj_gettimeofday(&tv);
    sdp->origin.user = pj_str("pjsip-siprtp");
    sdp->origin.version = sdp->origin.id = tv.sec + 2208988800UL;
    sdp->origin.net_type = pj_str("IN");
    sdp->origin.addr_type = pj_str("IP4");
    sdp->origin.addr = *pj_gethostname();
    sdp->name = pj_str("pjsip");

    /* Since we only support one media stream at present, put the
     * SDP connection line in the session level.
     */
    sdp->conn = (pjmedia_sdp_conn *)pj_pool_zalloc (pool, sizeof(pjmedia_sdp_conn));
    sdp->conn->net_type = pj_str("IN");
    sdp->conn->addr_type = pj_str("IP4");
    sdp->conn->addr = bind_addr_;

    /* SDP time and attributes. */
    sdp->time.start = sdp->time.stop = 0;
    sdp->attr_count = 0;

    /* Create media stream 0: */
    sdp->media_count = 2;
    m = (pjmedia_sdp_media *)pj_pool_zalloc (pool, sizeof(pjmedia_sdp_media));
    sdp->media[0] = m;

    /* Standard media info: */
    m->desc.media = pj_str("audio");
    m->desc.port = pj_ntohs(tpinfo.sock_info.rtp_addr_name.ipv4.sin_port);
    m->desc.port_count = 1;
    m->desc.transport = pj_str("RTP/AVP");

    /* Add format and rtpmap for each codec. */
    m->desc.fmt_count = 1;
    m->attr_count = 0;

	// audio rtpmap
	pjmedia_sdp_rtpmap rtpmap;
	char ptstr[10];

	sprintf(ptstr, "%d", g_audio_codecs[0].pt);
	pj_strdup2(pool, &m->desc.fmt[0], ptstr);
	rtpmap.pt = m->desc.fmt[0];
	rtpmap.clock_rate = g_audio_codecs[0].clock_rate;
	rtpmap.enc_name = pj_str(g_audio_codecs[0].name);
	rtpmap.param.slen = 0;

	pjmedia_sdp_rtpmap_to_attr(pool, &rtpmap, &attr);
	m->attr[m->attr_count++] = attr;

    /* Add sendrecv attribute. */
    attr = (pjmedia_sdp_attr *)pj_pool_zalloc(pool, sizeof(pjmedia_sdp_attr));
    attr->name = pj_str("sendrecv");
    m->attr[m->attr_count++] = attr;

	// Video media-attr
	pjmedia_transport_info_init(&tpinfo);
    pjmedia_transport_get_info(video->transport, &tpinfo);

	m = (pjmedia_sdp_media *)pj_pool_zalloc (pool, sizeof(pjmedia_sdp_media));
    sdp->media[1] = m;

	/* Standard media info: */
    m->desc.media = pj_str("video");
    m->desc.port = pj_ntohs(tpinfo.sock_info.rtp_addr_name.ipv4.sin_port);
    m->desc.port_count = 1;
    m->desc.transport = pj_str("RTP/AVP");

    /* Add format and rtpmap for each codec. */
    m->desc.fmt_count = 1;
    m->attr_count = 0;

	sprintf(ptstr, "%d", g_video_codecs[0].pt);
	pj_strdup2(pool, &m->desc.fmt[0], ptstr);
	rtpmap.pt = m->desc.fmt[0];
	rtpmap.clock_rate = g_video_codecs[0].clock_rate;
	rtpmap.enc_name = pj_str(g_video_codecs[0].name);
	rtpmap.param.slen = 0;

	pjmedia_sdp_rtpmap_to_attr(pool, &rtpmap, &attr);
	m->attr[m->attr_count++] = attr;

    /* Add sendrecv attribute. */
    attr = (pjmedia_sdp_attr *)pj_pool_zalloc(pool, sizeof(pjmedia_sdp_attr));
    attr->name = pj_str("sendrecv");
    m->attr[m->attr_count++] = attr;

    /* Done */
    *p_sdp = sdp;

    return PJ_SUCCESS;
}

const char *Session::GoodNumber(char *buf, pj_int32_t val)
{
    if (val < 1000)
	{
		pj_ansi_sprintf(buf, "%d", val);
    }
	else if(val < 1000000)
	{
		pj_ansi_sprintf(buf, "%d.%02dK", val / 1000, (val % 1000) / 100);
    }
	else
	{
		pj_ansi_sprintf(buf, "%d.%02dM", val / 1000000, (val % 1000000) / 10000);
    }

    return buf;
}

void Session::Statistic()
{
    int len;
	pjsip_inv_session *inv = invite_session();
    pjsip_dialog *dlg = inv->dlg;
    struct media_stream *audio = &medias_array()[0];
    char userinfo[128];
    char duration[80], last_update[80];
    char bps[16], ipbps[16], packets[16], bytes[16], ipbytes[16];
    unsigned decor;
    pj_time_val now;

    decor = pj_log_get_decor();
    pj_log_set_decor(PJ_LOG_HAS_NEWLINE);

    pj_gettimeofday(&now);

    /* Print duration */
    if (inv->state >= PJSIP_INV_STATE_CONFIRMED && connect_time().sec) {
		PJ_TIME_VAL_SUB(now, connect_time());

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
	      index(), pjsip_inv_state_name(inv->state), 
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

	if (invite_session() == NULL ||
		invite_session()->state < PJSIP_INV_STATE_CONFIRMED ||
		connect_time().sec == 0) 
    {
		pj_log_set_decor(decor);
		return;
    }

    /* Signaling quality */
    {
		char pdd[64], connectdelay[64];
		pj_time_val t;

		if (response_time().sec)
		{
			t = response_time();
			PJ_TIME_VAL_SUB(t, start_time());
			sprintf(pdd, "got 1st response in %ld ms", PJ_TIME_VAL_MSEC(t));
		}
		else
		{
			pdd[0] = '\0';
		}

		if (connect_time().sec)
		{
			t = connect_time();
			PJ_TIME_VAL_SUB(t, start_time());
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
		GoodNumber(bps, audio->bytes_per_frame * audio->clock_rate / audio->samples_per_frame),
		GoodNumber(ipbps, (audio->bytes_per_frame+32) * audio->clock_rate / audio->samples_per_frame)));

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
	   GoodNumber(packets, audio->rtcp.stat.rx.pkt),
	   GoodNumber(bytes, audio->rtcp.stat.rx.bytes),
	   GoodNumber(ipbytes, audio->rtcp.stat.rx.bytes + audio->rtcp.stat.rx.pkt * 32),
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
	   GoodNumber(packets, audio->rtcp.stat.tx.pkt),
	   GoodNumber(bytes, audio->rtcp.stat.tx.bytes),
	   GoodNumber(ipbytes, audio->rtcp.stat.tx.bytes + audio->rtcp.stat.tx.pkt * 32),
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
