#pragma once

#ifndef __AVS_PROXY_CLIENT_SCENE__
#define __AVS_PROXY_CLIENT_SCENE__

#include "Com.h"
#include "Parameter.h"

class TcpScene
{
public:
	TcpScene() {}
	virtual ~TcpScene() {}

	virtual void Maintain(TcpParameter *) {}
};

class UdpScene
{
public:
	UdpScene() {}
	virtual ~UdpScene() {}

	virtual void Maintain(UdpParameter *) {}
};

#endif
