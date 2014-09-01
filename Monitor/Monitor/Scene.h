#pragma once

#ifndef __AVS_PROXY_CLIENT_SCENE__
#define __AVS_PROXY_CLIENT_SCENE__

#include <memory>
#include "Com.h"
#include "Parameter.h"
#include "RoomTreeCtl.h"

using std::shared_ptr;

class TcpScene
{
public:
	TcpScene() {}
	virtual ~TcpScene() {}

	virtual void Maintain(shared_ptr<TcpParameter> ptr_tcp_param, RoomTreeCtl *room_ctl) {}
};

class UdpScene
{
public:
	UdpScene() {}
	virtual ~UdpScene() {}

	virtual void Maintain(shared_ptr<UdpParameter> ptr_udp_param) {}
};

#endif
