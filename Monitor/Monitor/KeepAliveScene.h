#ifndef __AVS_PROXY_CLIENT_KEEP_ALIVE_SCENE__
#define __AVS_PROXY_CLIENT_KEEP_ALIVE_SCENE__

#include "Parameter.h"
#include "Scene.h"

class KeepAliveParameter
	: public TcpParameter
{
public:
	KeepAliveParameter(const pj_uint8_t *, pj_uint16_t);
};

class KeepAliveScene
	: public TcpScene
{
public:
	KeepAliveScene() {}
	virtual ~KeepAliveScene() {}

	virtual void Maintain(TcpParameter *);
};

#endif
