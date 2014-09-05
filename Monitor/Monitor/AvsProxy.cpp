#include "stdafx.h"
#include "AvsProxy.h"

AvsProxy::AvsProxy(const pj_str_t &ip, pj_uint16_t port)
	: status_(AVS_PROXY_STATUS_OFFLINE)
	, ip_(pj_str(strdup(ip.ptr)))
	, port_(port)
{
}

pj_status_t AvsProxy::Login(pj_sock_t &sock)
{
	pj_status_t status = pj_open_tcp_clientport(&ip_, port_, sock);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	sock_ = sock;
	return status;
}