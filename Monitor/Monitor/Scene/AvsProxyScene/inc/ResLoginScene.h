#ifndef __AVS_PROXY_CLIENT_RES_LOGIN_SCENE__
#define __AVS_PROXY_CLIENT_RES_LOGIN_SCENE__

#include "Parameter.h"
#include "Scene.h"

class ResLoginParameter
	: public TcpParameter
{
public:
	ResLoginParameter(const pj_uint8_t *, pj_uint16_t);
};

class ResLoginScene
	: public TcpScene
{
public:
	ResLoginScene() {}
	virtual ~ResLoginScene() {}

	virtual void Maintain(shared_ptr<TcpParameter> ptr_tcp_param, AvsProxy *avs_proxy);
};

#endif
