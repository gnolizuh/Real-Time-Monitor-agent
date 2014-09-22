#include "stdafx.h"
#include "WatchsList.h"

void WatchsList::Cleanup()
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
				User *user = title_room->GetUser(param->user_id_);
				RETURN_IF_FAIL(user != nullptr);

				if (user->GetScreenIndex() != INVALID_SCREEN_INDEX)
				{
					// 如果这个用户已经在屏幕上显示
					RETURN_IF_FAIL(user->GetScreenIndex() >= 0 && user->screen_idx_ < MAXIMAL_SCREEN_NUM);
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
}

