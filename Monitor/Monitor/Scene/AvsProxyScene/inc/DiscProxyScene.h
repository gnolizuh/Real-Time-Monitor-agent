#ifndef __AVS_PROXY_CLIENT_DISC_PROXY_SCENE__
#define __AVS_PROXY_CLIENT_DISC_PROXY_SCENE__

#include "Screen.h"
#include "Scene.h"

class DiscProxyScene
	: public TcpScene
{
public:
	DiscProxyScene() {}
	virtual ~DiscProxyScene() {}

	virtual void Maintain(AvsProxy *&avs_proxy);
};

#endif
