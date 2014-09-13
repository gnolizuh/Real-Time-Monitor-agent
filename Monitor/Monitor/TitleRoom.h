#ifndef __AVS_PROXY_CLIENT_TITLE_ROOM__
#define __AVS_PROXY_CLIENT_TITLE_ROOM__

#include "pugixml.hpp"
#include "resource.h"
#include "Config.h"
#include "Node.h"
#include "AvsProxy.h"
#include "Com.h"

class TitleRoom;
class User
{
public:
	User(pj_int64_t user_id, TitleRoom *title_room)
		: tree_item_(nullptr)
		, screen_idx_(INVALID_SCREEN_INDEX)
		, user_id_(user_id)
		, title_room_(title_room)
		, audio_ssrc_(0)
		, video_ssrc_(0)
	{}

	inline pj_uint32_t GetScreenIndex() const
	{
		return screen_idx_;
	}

	inline void ConnectScreen(pj_uint32_t screen_idx)
	{
		screen_idx_ = screen_idx;
	}

	inline void DisconnectScreen()
	{
		screen_idx_ = INVALID_SCREEN_INDEX;
	}

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

	HTREEITEM   tree_item_;
	pj_uint32_t screen_idx_;
	pj_int64_t  user_id_;
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
	inline users_map_t &GetUsers()
	{
		return users_;
	}

	User *AddUser(pj_int64_t user_id);
	void  DelUser(pj_int64_t user_id);
	User *GetUser(pj_int64_t user_id);
	void  ModUser(User *user, pj_uint32_t audio_ssrc, pj_uint32_t video_ssrc);
	pj_status_t SendTCPPacket(const void *buf, pj_ssize_t *len);

protected:
	virtual void OnItemExpanded(CTreeCtrl &tree_ctrl, Node &parent);
	virtual void OnItemShrinked(CTreeCtrl &tree_ctrl, Node &parent);

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
