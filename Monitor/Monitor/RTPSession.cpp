#include "stdafx.h"
#include "RTPSession.h"

extern Config g_client_config;
RTPSession g_rtp_session;

pj_status_t RTPSession::Open()
{
	pj_status_t status;
	status = pj_open_udp_transport(&g_client_config.local_ip, g_client_config.local_media_port, rtp_sock_);
	RETURN_VAL_IF_FAIL( status == PJ_SUCCESS, status );

	status = pjmedia_rtp_session_init(&rtp_out_session_, RTP_EXPAND_PAYLOAD_TYPE, pj_rand());
	RETURN_VAL_IF_FAIL( status == PJ_SUCCESS, status );

	return PJ_SUCCESS;
}

void RTPSession::Close()
{
	pj_sock_close(rtp_sock_);
}

pj_status_t RTPSession::SendRTPPacket(pj_str_t &ip, pj_uint16_t port, const void *payload, pj_ssize_t payload_len)
{
	lock_guard<mutex> lock(rtp_lock_);

	RETURN_VAL_IF_FAIL(payload_len <= MAX_UDP_DATA_SIZE, PJ_EINVAL);

	pj_uint8_t packet[MAX_UDP_DATA_SIZE];
	const pjmedia_rtp_hdr *hdr;
	const void *p_hdr;
	int hdrlen;
	pj_ssize_t size;
	
	pj_status_t status;
	status = pjmedia_rtp_encode_rtp (&rtp_out_session_, RTP_EXPAND_PAYLOAD_TYPE, 0, payload_len, 0, &p_hdr, &hdrlen);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	hdr = (const pjmedia_rtp_hdr*) p_hdr;

	/* Copy RTP header to packet */
	pj_memcpy(packet, hdr, hdrlen);

	/* Copy RTP payload to packet */
	pj_memcpy(packet + hdrlen, payload, payload_len);

	/* Send RTP packet */
	size = hdrlen + payload_len;

	pj_sockaddr_in addr;
	status = pj_sockaddr_in_init(&addr, &ip, port);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	return pj_sock_sendto(rtp_sock_, packet, &size, 0, &addr, sizeof(addr));
}