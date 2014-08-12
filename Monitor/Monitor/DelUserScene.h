#ifndef __AVS_PROXY_CLIENT_DEL_USER_SCENE__
#define __AVS_PROXY_CLIENT_DEL_USER_SCENE__

#include "Parameter.h"
#include "Scene.h"

class DelUserParameter
	: public TcpParameter
{
public:
	DelUserParameter(const pj_uint8_t *, pj_uint16_t);

	pj_int32_t  room_id_;
	pj_int64_t  user_id_;
};

class DelUserScene
	: public TcpScene
{
public:
	DelUserScene() {}
	virtual ~DelUserScene() {}

	virtual void Maintain(TcpParameter *);
};

#endif
