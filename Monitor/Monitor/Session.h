#ifndef __MONITOR_SESSION__
#define __MONITOR_SESSION__

#include <vector>
#include "MessageQueue.hpp"
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
    unsigned		 sess_index;	    /* Session owner.		*/
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

using std::vector;
using sinashow::MessageQueue;
using sinashow::util_packet_type_t;
using sinashow::util_packet_t;

class Session
{
public:
	Session(pj_uint32_t);
	~Session();

	pj_status_t Prepare(pjsip_endpoint *, pjmedia_endpt *, pj_str_t, pj_uint16_t &, int);
	void        Launch();
	pj_status_t Invite(const pj_str_t *, const pj_str_t *, pj_int32_t);
	pj_status_t Hangup();
	pj_status_t Stop();
	void        UpdateMedia(pjsip_inv_session *, pj_status_t);

	void OnConnecting(pjsip_inv_session *, pjsip_event *);
	void OnConfirmed(pjsip_inv_session *, pjsip_event *);
	void OnDisconnected(pjsip_inv_session *, pjsip_event *);

private:
	static void OnRxRtp(void *, void *, pj_ssize_t);
	static void OnRxRtcp(void *, void *, pj_ssize_t);
	static void OnTimerStopSession(pj_timer_heap_t *, struct pj_timer_entry *);
	const char *GoodNumber(char *, pj_int32_t);
	pj_status_t CreateSdp(pj_pool_t *pool, pjmedia_sdp_session **);
	void        Statistic();

	inline pj_uint32_t index() const { return index_; }
	inline int module_id() const { return module_id_; }
	inline pjsip_inv_session *invite_session() const { return invite_session_; }
	inline unsigned medias_count() const { return medias_count_; }
	inline pj_str_t &bind_addr() { return bind_addr_; }
	inline pj_time_val &start_time() { return start_time_; }
	inline pj_time_val &response_time() { return response_time_; }
	inline pj_time_val &connect_time() { return connect_time_; }
	inline pjmedia_endpt *media_endpt() { return media_endpt_; }
	inline pjsip_endpoint *sip_endpt() { return sip_endpt_; }
	inline vector<struct media_stream> &medias_array() { return medias_array_; }
	inline MessageQueue<util_packet_t *> *msg_queue() { return msg_queue_; }

	inline void set_module_id(int module_id) { module_id_ = module_id; }
	inline void set_invite_session(pjsip_inv_session *invite_session) { invite_session_ = invite_session; }
	inline void set_bind_addr(pj_str_t bind_addr) { bind_addr_ = bind_addr; }
	inline void set_start_time(const pj_time_val &start_time) { start_time_ = start_time; }
	inline void set_response_time(const pj_time_val &response_time) { response_time_ = response_time; }
	inline void set_connect_time(const pj_time_val &connect_time) { connect_time_ = connect_time; }
	inline void set_media_endpt(pjmedia_endpt *media_endpt) { media_endpt_ = media_endpt; }
	inline void set_sip_endpt(pjsip_endpoint *sip_endpt) { sip_endpt_ = sip_endpt; }
	inline void set_medias_array(vector<struct media_stream> &medias_array) { medias_array_ = medias_array; }
	inline void set_msg_queue(MessageQueue<util_packet_t *> *msg_queue) { msg_queue_ = msg_queue; }

private:
	const pj_uint32_t    index_;
	int                  module_id_;
    pjsip_inv_session	*invite_session_;
    const unsigned       medias_count_;
	pj_str_t             bind_addr_;
    pj_time_val		     start_time_;
    pj_time_val		     response_time_;
    pj_time_val		     connect_time_;
	pjmedia_endpt       *media_endpt_;
	pjsip_endpoint      *sip_endpt_;
	vector<struct media_stream>	medias_array_;      /**< Support both audio and video. */
	MessageQueue<util_packet_t *> *msg_queue_;

	static struct codec g_audio_codecs[];
	static struct codec g_video_codecs[];
};

#endif
