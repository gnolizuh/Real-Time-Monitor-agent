#ifndef __MONITOR_SESSION__
#define __MONITOR_SESSION__

#include "common.h"
#include "UtilPacket.h"
#include "SessionMgr.h"

/* Codec descriptor: */
struct codec
{
    unsigned	pt;
    char*	    name;
    unsigned	clock_rate;
    unsigned	bit_rate;
    unsigned	ptime;
    char*	    description;
};

struct media_stream
{
    /* Static: */
    unsigned		 call_index;	    /* Call owner.		*/
    unsigned		 media_index;	    /* Media index in call.	*/
    pjmedia_transport   *transport;	    /* To send/recv RTP/RTCP	*/

    /* Active? */
    pj_bool_t		 active;	    /* Non-zero if is in call.	*/

    /* Current stream info: */
	union{
		pjmedia_stream_info	    ai;		    /* Current stream info.	*/
		pjmedia_vid_stream_info vi;
	};
    /* More info: */
    unsigned		 clock_rate;	    /* clock rate		*/
    unsigned		 samples_per_frame; /* samples per frame	*/
    unsigned		 bytes_per_frame;   /* frame size.		*/

    /* RTP session: */
    pjmedia_rtp_session	 out_sess;	    /* outgoing RTP session	*/
    pjmedia_rtp_session	 in_sess;	    /* incoming RTP session	*/

    /* RTCP stats: */
    pjmedia_rtcp_session rtcp;		    /* incoming RTCP session.	*/
};

using sinashow::util_packet_type_t;
using sinashow::util_packet_t;

class Session
{
public:
	Session();
	~Session();

	pj_status_t Prepare(pjsip_endpoint *, pjmedia_endpt *, pj_str_t, pj_uint16_t &, pj_uint8_t, int);
	void        Launch();
	pj_status_t Invite(const pj_str_t *, const pj_str_t *, pj_int32_t);
	pj_status_t Hangup();
	pj_status_t Stop();
	void        UpdateMedia(pjsip_inv_session *, pj_status_t);

	unsigned		     index;
	int                  mod_id;
    pjsip_inv_session	*inv_session;
    unsigned		     media_count;
    struct media_stream	 medias[2];      /**< Support both audio and video. */
	pj_str_t             local_addr;
    pj_time_val		     start_time;
    pj_time_val		     response_time;
    pj_time_val		     connect_time;
	pjmedia_endpt       *media_endpt;
	pjsip_endpoint      *sip_endpt;

	void OnConnecting(pjsip_inv_session *, pjsip_event *);
	void OnConfirmed(pjsip_inv_session *, pjsip_event *);
	void OnDisconnected(pjsip_inv_session *, pjsip_event *);

private:
	static void OnRxRtp(void *, void *, pj_ssize_t);
	static void OnRxRtcp(void *, void *, pj_ssize_t);

	static void OnTimerStopSession(pj_timer_heap_t *, struct pj_timer_entry *);
	const char *GoodNumber(char *, pj_int32_t);
	pj_status_t CreateSdp(pj_pool_t *pool, pjmedia_sdp_session **);
	void Statistic();

	static struct codec audio_codecs[];
	static struct codec video_codecs[];
};

#endif
