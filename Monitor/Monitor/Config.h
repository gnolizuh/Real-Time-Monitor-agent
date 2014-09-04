#ifndef __AVS_PROXY_CLIENT_CONFIG__
#define __AVS_PROXY_CLIENT_CONFIG__

#include "Com.h"

class Config
{
public:
	pj_str_t    local_ip;
	pj_uint16_t local_media_port;
	pj_str_t    log_file_name;
	pj_str_t    tls_host;
	pj_str_t    tls_uri;
};

#endif
