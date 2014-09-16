#ifndef __AVS_PROXY_CLIENT_RTP_SESSION__
#define __AVS_PROXY_CLIENT_RTP_SESSION__

#include <mutex>

#include "Config.h"
#include "Com.h"

using std::mutex;

class RTPSession
{
public:
	pj_status_t Open();
	void        Close();
	pj_status_t SendRTPPacket(pj_str_t &ip, pj_uint16_t port, const void *payload, pj_ssize_t payload_len);
	inline pj_sock_t GetRTPSock() const { return rtp_sock_; }

private:
	pj_sock_t           rtp_sock_;
	mutex               rtp_lock_;
	pjmedia_rtp_session rtp_out_session_;
};

#endif