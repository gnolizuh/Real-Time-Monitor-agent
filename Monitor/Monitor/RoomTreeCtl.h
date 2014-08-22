#ifndef __AVS_PROXY_CLIENT_ROOM_TREE_CTL__
#define __AVS_PROXY_CLIENT_ROOM_TREE_CTL__

#include <mutex>
#include <map>
#include "Com.h"
#include "resource.h"

using std::lock_guard;
using std::mutex;
using std::map;

class User
{
public:
	HTREEITEM   tree_item_;
	pj_int64_t  user_id_;
	pj_int32_t  room_id_;
	pj_uint32_t audio_ssrc_;
	pj_uint32_t video_ssrc_;
};

typedef map<pj_int64_t, User *> users_map_t;
class Room
{
public:
	HTREEITEM   tree_item_;
	pj_int32_t  room_id_;
	users_map_t users_;
};

typedef map<pj_int32_t, Room *> rooms_map_t;
class RoomTreeCtl
	: public Noncopyable
	, public CTreeCtrl
{
public:
	RoomTreeCtl();

	pj_status_t Prepare(const CWnd *wrapper, pj_uint32_t uid);
	pj_status_t Launch();
	void        Destory();
	Room       *AddRoom(pj_int32_t room_id);
	void        DelRoom(pj_int32_t room_id);
	Room       *GetRoom(pj_int32_t room_id);
	User       *AddUser(Room *room, pj_int64_t user_id);
	void        DelUser(Room *room, pj_int64_t user_id);
	User       *GetUser(Room *room, pj_int64_t user_id);
	void        ModMedia(User *user, pj_uint32_t audio_ssrc, pj_uint32_t video_ssrc);
	void        MoveToRect(const CRect &rect);
	void        HideWindow();

protected:
	afx_msg void OnTvnBeginDrag(NMHDR *pNMHDR, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()

private:
	HTREEITEM   root_item_;
	mutex       rooms_mutex_;
	rooms_map_t rooms_;
};

#endif
