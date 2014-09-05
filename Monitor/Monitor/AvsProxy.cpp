#include "stdafx.h"
#include "AvsProxy.h"

AvsProxy::AvsProxy(const pj_str_t &ip, pj_uint16_t port)
	: status_(AVS_PROXY_STATUS_OFFLINE)
	, ip_(pj_str(strdup(ip.ptr)))
	, port_(port)
{
}

pj_status_t AvsProxy::LinkRoom()
{
	
}