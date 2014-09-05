#ifndef __AVS_PROXY_CLIENT_AVS_PROXY__
#define __AVS_PROXY_CLIENT_AVS_PROXY__

#include "Com.h"

enum _enum_avs_proxy_status_
{
	AVS_PROXY_STATUS_OFFLINE = 0,
	AVS_PROXY_STATUS_LOGINING,
	AVS_PROXY_STATUS_ONLINE
};

class AvsProxy
{
public:
	AvsProxy(const pj_str_t &ip, pj_uint16_t port);
	pj_status_t LinkRoom();

	struct event *tcp_ev_;
	pj_sock_t    sock_;
	pj_uint8_t   status_;
	pj_str_t     ip_;
	pj_uint16_t  port_;
	pj_bool_t    active_; 
	pj_uint8_t   tcp_storage_[MAX_STORAGE_SIZE];  // TCP»º´æ
	pj_uint16_t  tcp_storage_offset_;             // TCP»º´æÆ«ÒÆµØÖ·
};

#endif
