#ifndef __AVS_PROXY_CLIENT_AVS_PROXY__
#define __AVS_PROXY_CLIENT_AVS_PROXY__

#include "AvsProxyStructs.h"
#include "TitleRoom.h"
#include "Screen.h"
#include "Config.h"
#include "Com.h"

enum _enum_avs_proxy_status_
{
	AVS_PROXY_STATUS_OFFLINE = 0,
	AVS_PROXY_STATUS_LOGINING,
	AVS_PROXY_STATUS_ONLINE
};

class User;
class TitleRoom;
typedef map<pj_int32_t, TitleRoom *> room_map_t;
class AvsProxy
{
public:
	AvsProxy(pj_uint16_t id, const pj_str_t &ip, pj_uint16_t tcp_port, pj_uint16_t udp_port, pj_sock_t sock);
	pj_status_t Login();
	pj_status_t Logout();

	/**< 当上麦用户被拖拽到Screen中 */
	pj_status_t LinkRoomUser(User *user);
	pj_status_t UnlinkRoomUser(User *user);
	pj_status_t LinkRoom(TitleRoom *title_room);
	pj_status_t UnlinkRoom(TitleRoom *title_room);
	pj_status_t AddRoom(pj_int32_t room_id, TitleRoom *title_room);
	pj_status_t DelRoom(pj_int32_t room_id, TitleRoom *title_room);
	pj_status_t GetRoom(pj_int32_t room_id, TitleRoom *&title_room);
	pj_uint32_t GetRoomSize();
	pj_status_t SendTCPPacket(const void *buf, pj_ssize_t *len);

	ev_function_t *pfunction_;
	struct event *tcp_ev_;
	pj_sock_t    sock_;
	pj_uint8_t   status_;
	pj_uint16_t  id_;
	pj_str_t     ip_;
	mutex        tcp_lock_;
	pj_uint16_t  tcp_port_;
	pj_uint16_t  udp_port_;
	pj_bool_t    active_;
	mutex        rooms_lock_;
	room_map_t   rooms_;
	pj_uint8_t   tcp_storage_[MAX_STORAGE_SIZE];  // TCP缓存
	pj_uint16_t  tcp_storage_offset_;             // TCP缓存偏移地址
};

#endif
