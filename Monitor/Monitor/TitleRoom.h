#ifndef __AVS_PROXY_CLIENT_TITLE_ROOM__
#define __AVS_PROXY_CLIENT_TITLE_ROOM__

#include "pugixml.hpp"
#include "resource.h"
#include "Config.h"
#include "Node.h"
#include "Screen.h"
#include "AvsProxy.h"
#include "WatchsList.h"
#include "Com.h"

class Screen;
class TitleRoom;
class User
	: public Node
{
public:
	User(pj_int64_t user_id, pj_uint32_t mic_id, TitleRoom *title_room)
		: Node(0, pj_str(""), 0, 0, TITLE_USER)
		, user_id_(user_id)
		, mic_id_(mic_id)
		, screen_(nullptr)
		, screen_idx_(INVALID_SCREEN_INDEX)
		, title_room_(title_room)
		, audio_ssrc_(0)
		, video_ssrc_(0)
	{}

	void ConnectScreen(Screen *screen, pj_uint32_t screen_idx);
	void DisconnectScreen();
	void ModMedia(pj_uint32_t audio_ssrc, pj_uint32_t video_ssrc);

	inline bool operator!=(const User &user) const
	{
		return !operator==(user);
	}

	inline bool operator==(const User &user) const
	{
		return (user_id_ == user.user_id_
			&& title_room_ == user.title_room_
			&& audio_ssrc_ == user.audio_ssrc_
			&& video_ssrc_ == user.video_ssrc_);
	}

	pj_int64_t  user_id_;
	pj_uint32_t mic_id_;
	Screen     *screen_;
	pj_uint32_t screen_idx_;
	TitleRoom  *title_room_;
	pj_uint32_t audio_ssrc_;
	pj_uint32_t video_ssrc_;
};

class AvsProxy;
typedef map<pj_int64_t, User *> users_map_t;
class TitleRoom
	: public Node
{
public:
	TitleRoom(CTreeCtrl *tree_ctrl, pj_int32_t id, const pj_str_t &name, pj_uint32_t order, pj_uint32_t usercount);
	virtual ~TitleRoom();

	void OnCreate(AvsProxy *proxy);
	virtual void OnDestory();
	User *AddUser(pj_int64_t user_id, pj_uint32_t mic_id);
	void  DelUser(pj_int64_t user_id, users_map_t::iterator &puser);
	User *GetUser(pj_int64_t user_id);
	void  ModUser(User *user, pj_uint32_t audio_ssrc, pj_uint32_t video_ssrc);
	pj_status_t SendTCPPacket(const void *buf, pj_ssize_t *len);
	pj_status_t OnShowPage(pj_uint32_t &offset, pj_uint32_t &first);
	void IncreaseCount(pj_uint32_t &user_count);
	virtual void OnWatched(void *ctrl);

protected:
	virtual void OnItemExpanded(CTreeCtrl &tree_ctrl);
	virtual void OnItemShrinked(CTreeCtrl &tree_ctrl);

private:
	void AddNode(pj_int32_t id, const pj_str_t &name, pj_uint32_t order, pj_uint32_t usercount);
	void DelNode(pj_int32_t id);

public:
	CTreeCtrl  *tree_ctrl_;
	AvsProxy   *proxy_;
	mutex       room_lock_;
	users_map_t users_;
};

#endif
