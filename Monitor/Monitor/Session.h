#ifndef __MONITOR_SESSION__
#define __MONITOR_SESSION__

#include "common.h"

struct media_stream
{
    /* Static: */
    unsigned		 call_index;	    /* Call owner.		*/
    unsigned		 media_index;	    /* Media index in call.	*/
    pjmedia_transport   *transport;	    /* To send/recv RTP/RTCP	*/

    /* Active? */
    pj_bool_t		 active;	    /* Non-zero if is in call.	*/

    /* Current stream info: */
    pjmedia_stream_info	 ai;		    /* Current stream info.	*/
	pjmedia_vid_stream_info vi;

    /* More info: */
    unsigned		 clock_rate;	    /* clock rate		*/
    unsigned		 samples_per_frame; /* samples per frame	*/
    unsigned		 bytes_per_frame;   /* frame size.		*/

    /* RTP session: */
    pjmedia_rtp_session	 out_sess;	    /* outgoing RTP session	*/
    pjmedia_rtp_session	 in_sess;	    /* incoming RTP session	*/

    /* RTCP stats: */
    pjmedia_rtcp_session rtcp;		    /* incoming RTCP session.	*/

    /* Thread: */
    pj_bool_t		 thread_quit_flag;  /* Stop media thread.	*/
    pj_thread_t		*thread;	    /* Media thread.		*/
};

class Session
{
public:
	Session();
	~Session();

	pj_status_t Prepare(pjsip_endpoint *, pjmedia_endpt *, pj_str_t, pj_uint16_t &, pj_uint8_t, int);
	void Launch();
	pj_status_t Start(const pj_str_t *, const pj_str_t *, pj_int32_t);
	void Stop();
	void Hangup();

	unsigned		     index;
	int                  mod_id;
	long                 duration;
    pjsip_inv_session	*inv_session;
    unsigned		     media_count;
    struct media_stream	 medias[2];      /**< Support both audio and video. */
    pj_time_val		     start_time;
    pj_time_val		     response_time;
    pj_time_val		     connect_time;
    pj_timer_entry	     d_timer;	    /**< Disconnect timer.	*/
	pjmedia_endpt       *media_endpt;
	pjsip_endpoint      *sip_endpt;

	void OnConnecting();
	void OnConfirmed();
	void OnDisconnected();

private:
	const char *good_number(char *, pj_int32_t);
	void Statistic();
};

#endif
