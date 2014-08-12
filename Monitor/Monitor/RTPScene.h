#ifndef __AVS_PROXY_CLIENT_RTP_SCENE__
#define __AVS_PROXY_CLIENT_RTP_SCENE__

#include "Parameter.h"
#include "Scene.h"

class RTPParameter
	: public UdpParameter
{
public:
	RTPParameter();
};

class RTPScene
	: public UdpScene
{
public:
	RTPScene() {}
	virtual ~RTPScene() {}

	virtual void Maintain(TcpParameter *);
};

#endif
