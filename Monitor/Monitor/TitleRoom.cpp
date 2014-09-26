#include "stdafx.h"
#include "TitleRoom.h"

#ifdef __ABS_FILE__
#undef __ABS_FILE__
#endif

#define __ABS_FILE__ "TitleRoom.cpp"

void User::ConnectScreen(Screen *screen, pj_uint32_t screen_idx)
{
	if (audio_ssrc_ > 0)
	{
		g_av_index_map[AUDIO_INDEX].insert(index_map_t::value_type(audio_ssrc_, screen_idx));
	}

	if (video_ssrc_ > 0)
	{
		g_av_index_map[VIDEO_INDEX].insert(index_map_t::value_type(video_ssrc_, screen_idx));
	}

	screen_ = screen;
	screen_idx_ = screen_idx;
}

void User::DisconnectScreen()
{
	g_av_index_map[AUDIO_INDEX].erase(audio_ssrc_);
	g_av_index_map[VIDEO_INDEX].erase(video_ssrc_);

	screen_ = nullptr;
	screen_idx_ = INVALID_SCREEN_INDEX;
}

void User::ModMedia(pj_uint32_t audio_ssrc, pj_uint32_t video_ssrc)
{
	audio_ssrc_ = audio_ssrc;
	video_ssrc_ = video_ssrc;

	if (screen_ != nullptr)
	{
		if (audio_ssrc_ > 0)
		{
			g_av_index_map[AUDIO_INDEX].insert(index_map_t::value_type(audio_ssrc_, screen_idx_));
		}

		if (video_ssrc_ > 0)
		{
			g_av_index_map[VIDEO_INDEX].insert(index_map_t::value_type(video_ssrc_, screen_idx_));
		}
	}
}

TitleRoom::TitleRoom(CTreeCtrl *tree_ctrl, pj_int32_t id, const pj_str_t &name, order_t order, pj_uint32_t usercount)
	: Node(id, name, order, usercount, TITLE_ROOM)
	, tree_ctrl_(tree_ctrl)
{
}

TitleRoom::~TitleRoom()
{
}

void TitleRoom::OnCreate(AvsProxy *proxy)
{
	proxy_ = proxy;
}

void TitleRoom::OnDestory()
{
	users_map_t::iterator puser = users_.begin();
	for (; puser != users_.end();)
	{
		users_map_t::mapped_type user = puser->second;
		if (user != nullptr)
		{
			DelUser(user->user_id_, puser);
		}
	}

	proxy_ = nullptr;

	PJ_LOG(5, (__ABS_FILE__, "Room[%d] was destoryed!", id_));
}

void TitleRoom::OnWatched(void *ctrl)
{
	if(proxy_ != nullptr)
	{
		g_watchs_list.AddRoom(this);
	}
	else
	{
		sinashow::SendMessage(WM_EXPANDEDROOM, (WPARAM)ctrl, (LPARAM)this);
	}
}

void TitleRoom::OnItemExpanded(CTreeCtrl &tree_ctrl)
{
	lock_guard<mutex> lock(room_lock_);

	RETURN_IF_FAIL(proxy_ == nullptr);

	sinashow::SendMessage(WM_EXPANDEDROOM, (WPARAM)nullptr, (LPARAM)this);
}

void TitleRoom::OnItemShrinked(CTreeCtrl &tree_ctrl)
{
	lock_guard<mutex> lock(room_lock_);
	pj_bool_t useless = PJ_FALSE;
	for (users_map_t::iterator puser = users_.begin();
		puser != users_.end(); ++ puser)
	{
		users_map_t::mapped_type user = puser->second;
		if(user != nullptr)
		{
			if (user->screen_idx_ != INVALID_SCREEN_INDEX)
			{
				useless = PJ_TRUE;
				break;
			}
		}
	}

	RETURN_IF_FAIL(useless == PJ_FALSE);

	sinashow::SendMessage(WM_SHRINKEDROOM, (WPARAM)0, (LPARAM)this);
}

User *TitleRoom::AddUser(pj_int64_t user_id, pj_uint32_t mic_id)
{
	lock_guard<mutex> lock(room_lock_);
	if(users_.empty())
	{
		DelAll(*tree_ctrl_);
	}

	PJ_LOG(5, (__ABS_FILE__, "Room[%d] add a new user[%ld]", id_, user_id));

	users_map_t::iterator puser = users_.find(user_id);
	users_map_t::mapped_type user = nullptr;
	if(puser != users_.end())
	{
		user = puser->second;
		user->mic_id_ = mic_id;
	}
	else
	{
		user = new User(user_id, mic_id, this);
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

void TitleRoom::DelUser(pj_int64_t user_id, users_map_t::iterator &puser)
{
	lock_guard<mutex> lock(room_lock_);
	puser = users_.find(user_id);
	users_map_t::mapped_type user = nullptr;
	if(puser != users_.end())
	{
		user = puser->second;
		puser = users_.erase(puser);
	}

	PJ_LOG(5, (__ABS_FILE__, "Room[%d] delete an old user[%ld]", id_, user_id));

	if(user != nullptr)
	{
		tree_ctrl_->DeleteItem(user->tree_item_);

		Screen *screen = user->screen_;
		if (screen != nullptr)
		{
			screen->DisconnectUser();
		}

		delete user;
		user = nullptr;
	}

	if(users_.empty())
	{
		AddNull(*tree_ctrl_);
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
	else
	{
		user = AddUser(user_id, 0);
	}

	return user;
}

void TitleRoom::ModUser(User *user, pj_uint32_t audio_ssrc, pj_uint32_t video_ssrc)
{
	RETURN_IF_FAIL(user != nullptr);

	lock_guard<mutex> lock(room_lock_);
	user->ModMedia(audio_ssrc, video_ssrc);

	PJ_LOG(5, (__ABS_FILE__, "Room[%d] modify an old user[%ld] audio_ssrc[%u] video_ssrc[%u]",
		id_, user->user_id_, user->audio_ssrc_, user->video_ssrc_));
}

void TitleRoom::IncreaseCount(pj_uint32_t &user_count)
{
	lock_guard<mutex> lock(room_lock_);
	user_count += users_.size();
}

pj_status_t TitleRoom::OnShowPage(pj_uint32_t &offset, pj_uint32_t &first)
{
	lock_guard<mutex> lock(room_lock_);
	users_map_t::iterator puser = users_.begin();
	for (; puser != users_.end(); ++puser)
	{
		users_map_t::mapped_type user = puser->second;
		if (offset >= first)
		{
			pj_uint32_t idx = offset - first;
			sinashow::SendMessage(WM_WATCH_ROOM_USER, (WPARAM)user, (LPARAM)idx);
		}

		if (++ offset >= (first + MAXIMAL_SCREEN_NUM))
		{
			return PJ_EINVAL;
		}
	}

	return PJ_SUCCESS;
}

pj_status_t TitleRoom::SendTCPPacket(const void *buf, pj_ssize_t *len)
{
	RETURN_VAL_IF_FAIL(proxy_ != nullptr, PJ_EINVAL);
	return proxy_->SendTCPPacket(buf, len);
}
