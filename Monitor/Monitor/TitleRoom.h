#ifndef __AVS_PROXY_CLIENT_TITLE_ROOM__
#define __AVS_PROXY_CLIENT_TITLE_ROOM__

#include "pugixml.hpp"
#include "resource.h"
#include "Config.h"
#include "Node.h"
#include "AvsProxy.h"
#include "Com.h"

class User
{
public:
	inline bool operator!=(const User &user) const
	{
		return !operator==(user);
	}

	inline bool operator==(const User &user) const
	{
		return (user_id_ == user.user_id_
			&& room_id_ == user.room_id_
			&& audio_ssrc_ == user.audio_ssrc_
			&& video_ssrc_ == user.video_ssrc_);
	}

	HTREEITEM   tree_item_;
	pj_int64_t  user_id_;
	pj_int32_t  room_id_;
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
	virtual void OnItemExpanded(CTreeCtrl &tree_ctrl, Node &parent);
	User *AddUser(pj_int64_t user_id);
	void  DelUser(pj_int64_t user_id);
	User *GetUser(pj_int64_t user_id);
	void  ModUser(User *user, pj_uint32_t audio_ssrc, pj_uint32_t video_ssrc);
	pj_status_t SendTCPPacket(const void *buf, pj_ssize_t *len);

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
