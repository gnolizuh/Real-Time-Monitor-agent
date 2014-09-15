#ifndef __AVS_PROXY_CLIENT_NAT_SCENE__
#define __AVS_PROXY_CLIENT_NAT_SCENE__

#include "Parameter.h"
#include "Scene.h"

class NATParameter
	: public UdpParameter
{
public:
	NATParameter(const pj_uint8_t *, pj_uint16_t);

	pj_uint16_t client_id_;
};

class NATScene
	: public UdpScene
{
public:
	NATScene() {}
	virtual ~NATScene() {}

	virtual void Maintain(shared_ptr<UdpParameter> ptr_udp_param, AvsProxy *avs_proxy);
};

#endif
