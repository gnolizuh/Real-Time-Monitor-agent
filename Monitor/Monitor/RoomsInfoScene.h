#ifndef __AVS_PROXY_CLIENT_ROOMS_INFO_SCENE__
#define __AVS_PROXY_CLIENT_ROOMS_INFO_SCENE__

#include <vector>

#include "Parameter.h"
#include "Scene.h"

using std::vector;

#pragma pack(1)
typedef struct
{
	pj_int64_t  user_id_;
	pj_uint32_t audio_ssrc_;
	pj_uint32_t video_ssrc_;
} user_info_t;

typedef struct
{
	pj_int32_t room_id_;
	pj_uint32_t user_count_;
	vector<user_info_t> users_info_;
} room_info_t;

class RoomsInfoParameter
	: public TcpParameter
{
public:
	RoomsInfoParameter(const pj_uint8_t *, pj_uint16_t);

	pj_uint32_t room_count_;
	vector<room_info_t> rooms_info_;
};
#pragma pack()

class RoomsInfoScene
	: public TcpScene
{
public:
	RoomsInfoScene() {}
	virtual ~RoomsInfoScene() {}

	virtual void Maintain(TcpParameter *, RoomTreeCtl *);
};

#endif
