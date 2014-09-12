#include "stdafx.h"
#include "TitleRoom.h"

extern Config g_client_config;

TitleRoom::TitleRoom(CTreeCtrl *tree_ctrl, pj_int32_t id, const pj_str_t &name, order_t order, pj_uint32_t usercount)
	: Node(id, name, order, usercount, TITLE_ROOM)
	, tree_ctrl_(tree_ctrl)
{
}

TitleRoom::~TitleRoom()
{
}

void TitleRoom::OnItemExpanded(CTreeCtrl &tree_ctrl, Node &parent)
{
	//Let the mainframe knowns.
	::SendMessage(AfxGetMainWnd()->m_hWnd, WM_EXPANDEDROOM, 0, (LPARAM)this);
}

User *TitleRoom::AddUser(pj_int64_t user_id)
{
	lock_guard<mutex> lock(room_lock_);
	if(users_.empty())
	{
		DelAll(*tree_ctrl_, *this);
	}

	users_map_t::iterator puser = users_.find(user_id);
	users_map_t::mapped_type user = nullptr;
	if(puser != users_.end())
	{
		user = puser->second;
	}
	else
	{
		user = new User();
		user->user_id_ = user_id;
		user->room_id_ = id_;
		users_[user_id] = user;

		wchar_t str_user_id[32];
		swprintf(str_user_id, sizeof(str_user_id) - 1, L"%u", user_id);

		TVINSERTSTRUCT tvInsert;
		tvInsert.hParent = tree_item_;
		tvInsert.hInsertAfter = TVI_LAST;
		tvInsert.item.lParam = (LPARAM)user;
		tvInsert.item.pszText = str_user_id;
		tvInsert.item.mask = LVIF_TEXT | TVIF_PARAM;

		user->tree_item_ = tree_ctrl_->InsertItem(&tvInsert);
	}

	return user;
}

void TitleRoom::DelUser(pj_int64_t user_id)
{
	lock_guard<mutex> lock(room_lock_);
	users_map_t::iterator puser = users_.find(user_id);
	users_map_t::mapped_type user = nullptr;
	if(puser != users_.end())
	{
		user = puser->second;
		users_.erase(puser);
	}

	if(user != nullptr)
	{
		tree_ctrl_->DeleteItem(user->tree_item_);
		delete user;
		user = nullptr;
	}

	if(users_.empty())
	{
		AddNull(*tree_ctrl_, tree_item_);
	}
}

User *TitleRoom::GetUser(pj_int64_t user_id)
{
	lock_guard<mutex> lock(room_lock_);
	users_map_t::iterator puser = users_.find(user_id);
	users_map_t::mapped_type user = nullptr;
	if(puser != users_.end())
	{
		user = puser->second;
	}

	return user;
}

void TitleRoom::ModUser(User *user, pj_uint32_t audio_ssrc, pj_uint32_t video_ssrc)
{
	lock_guard<mutex> lock(room_lock_);
	user->audio_ssrc_ = audio_ssrc;
	user->video_ssrc_ = video_ssrc;
}

pj_status_t TitleRoom::SendTCPPacket(const void *buf, pj_ssize_t *len)
{
	RETURN_VAL_IF_FAIL(proxy_ != nullptr, PJ_EINVAL);
	return proxy_->SendTCPPacket(buf, len);
}
