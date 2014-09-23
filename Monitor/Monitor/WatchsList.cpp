#include "stdafx.h"
#include "WatchsList.h"

WatchsList g_watchs_list;

WatchsList::WatchsList()
	: watching_(PJ_FALSE)
{
}

void WatchsList::Begin()
{
	watching_ = PJ_TRUE;
}

void WatchsList::End()
{
	watched_users_list_t::iterator puser = bewatched_users_list_.begin();
	for(; puser != bewatched_users_list_.end(); ++ puser)
	{
		watched_users_list_t::value_type user = *puser;
		if(user != nullptr)
		{
			TitleRoom *title_room = user->title_room_;
			if(title_room != nullptr)
			{
				AvsProxy *proxy = title_room->proxy_;

				if (user->GetScreenIndex() >= 0 && user->GetScreenIndex() < MAXIMAL_SCREEN_NUM)
				{
					// 如果这个用户已经在屏幕上显示
					Screen *screen = g_screens[user->screen_idx_];

					RETURN_IF_FAIL(screen != nullptr);
					screen->DisconnectUser();
					user->DisconnectScreen();

					g_av_index_map[AUDIO_INDEX].erase(user->audio_ssrc_);
					g_av_index_map[VIDEO_INDEX].erase(user->video_ssrc_);
				}
			}
		}
	}

	watching_ = PJ_FALSE;
}

void WatchsList::OnAddUser(User *user)
{
	RETURN_IF_FAIL(watching_ == PJ_TRUE);
}

void WatchsList::OnDelUser(User *user)
{
	RETURN_IF_FAIL(watching_ == PJ_TRUE);
}

void WatchsList::OnModUser(User *user, pj_uint32_t audio_ssrc, pj_uint32_t video_ssrc)
{
	RETURN_IF_FAIL(watching_ == PJ_TRUE);
}
