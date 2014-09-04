#ifndef __AVS_PROXY_CLIENT_MOD_MEDIA_SCENE__
#define __AVS_PROXY_CLIENT_MOD_MEDIA_SCENE__

#include "Parameter.h"
#include "Scene.h"

class ModMediaParameter
	: public TcpParameter
{
public:
	ModMediaParameter(const pj_uint8_t *, pj_uint16_t);

	pj_int32_t  room_id_;
	pj_int64_t  user_id_;
	pj_uint32_t audio_ssrc_;
	pj_uint32_t video_ssrc_;
};

class ModMediaScene
	: public TcpScene
{
public:
	ModMediaScene() {}
	virtual ~ModMediaScene() {}

	virtual void Maintain(shared_ptr<TcpParameter> ptr_tcp_param, Node *node);
};

#endif
