#ifndef __AVS_PROXY_CLIENT_CONFIG__
#define __AVS_PROXY_CLIENT_CONFIG__

#include <mutex>
#include "Com.h"

using std::lock_guard;
using std::mutex;

class Config
{
public:
	pj_uint16_t client_id;
	pj_str_t    local_ip;
	pj_uint16_t local_media_port;
	pj_str_t    log_file_name;
	pj_str_t    tls_host;
	pj_uint16_t tls_port;
	pj_str_t    tls_uri;
	pj_str_t    rrtvms_fcgi_host;
	pj_uint16_t rrtvms_fcgi_port;
	pj_str_t    rrtvms_fcgi_uri;
};

#endif
