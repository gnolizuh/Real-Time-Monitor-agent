#include "stdafx.h"
#include "TitleRoom.h"

extern Config g_client_config;
extern Screen *g_screens[MAXIMAL_SCREEN_NUM];
extern index_map_t g_av_index_map[2];

#ifdef __ABS_FILE__
#undef __ABS_FILE__
#endif

#define __ABS_FILE__ "TitleRoom.cpp"

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
			if (user->GetScreenIndex() != INVALID_SCREEN_INDEX)
			{
				// 如果这个用户已经在屏幕上显示
				if (user->GetScreenIndex() >= 0 && user->screen_idx_ < MAXIMAL_SCREEN_NUM)
				{
					Screen *screen = g_screens[user->screen_idx_];

					if (screen != nullptr) // 清除user和screen之间的联系
					{
						screen->DisconnectUser();
						user->DisconnectScreen();

						g_av_index_map[AUDIO_INDEX].erase(user->audio_ssrc_);
						g_av_index_map[VIDEO_INDEX].erase(user->video_ssrc_);
					}
				}
			}
		}

		DelUser(user->user_id_, puser); // 然后删除这个用户
	}

	proxy_ = nullptr;

	PJ_LOG(5, (__ABS_FILE__, "Room[%d] was destoryed!", id_));
}

void TitleRoom::OnItemExpanded(CTreeCtrl &tree_ctrl, Node &parent)
{
	lock_guard<mutex> lock(room_lock_);

	RETURN_IF_FAIL(proxy_ == nullptr);

	::SendMessage(AfxGetMainWnd()->m_hWnd, WM_EXPANDEDROOM, 0, (LPARAM)this);
}

void TitleRoom::OnItemShrinked(CTreeCtrl &tree_ctrl, Node &parent)
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

	::SendMessage(AfxGetMainWnd()->m_hWnd, WM_SHRINKEDROOM, 0, (LPARAM)this);
}

User *TitleRoom::AddUser(pj_int64_t user_id, pj_uint32_t mic_id)
{
	lock_guard<mutex> lock(room_lock_);
	if(users_.empty())
	{
		DelAll(*tree_ctrl_, *this);
	}

	PJ_LOG(5, (__ABS_FILE__, "Room[%d] add a new user[%ld]", id_, user_id));

	users_map_t::iterator puser = users_.find(user_id);
	users_map_t::mapped_type user = nullptr;
	if(puser != users_.end())
	{
		user = puser->second;
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
	RETURN_IF_FAIL(user != nullptr);

	lock_guard<mutex> lock(room_lock_);
	user->audio_ssrc_ = audio_ssrc;
	user->video_ssrc_ = video_ssrc;

	PJ_LOG(5, (__ABS_FILE__, "Room[%d] modify an old user[%ld] audio_ssrc[%u] video_ssrc[%u]",
		id_, user->user_id_, audio_ssrc, video_ssrc));
}

pj_status_t TitleRoom::SendTCPPacket(const void *buf, pj_ssize_t *len)
{
	RETURN_VAL_IF_FAIL(proxy_ != nullptr, PJ_EINVAL);
	return proxy_->SendTCPPacket(buf, len);
}
