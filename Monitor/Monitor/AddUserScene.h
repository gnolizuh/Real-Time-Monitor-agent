#ifndef __AVS_PROXY_CLIENT_ADD_USER_SCENE__
#define __AVS_PROXY_CLIENT_ADD_USER_SCENE__

#include "Parameter.h"
#include "Scene.h"

class AddUserParameter
	: public TcpParameter
{
public:
	AddUserParameter(const pj_uint8_t *, pj_uint16_t);

	pj_int32_t  room_id_;
	pj_int64_t  user_id_;
};

class AddUserScene
	: public TcpScene
{
public:
	AddUserScene() {}
	virtual ~AddUserScene() {}

	virtual void Maintain(TcpParameter *, RoomTreeCtl *);
};

#endif
