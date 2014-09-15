#include "stdafx.h"
#include "DelUserScene.h"

extern index_map_t g_av_index_map[2];
extern Screen *g_screens[MAXIMAL_SCREEN_NUM];

DelUserParameter::DelUserParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: TcpParameter(storage, storage_len)
{
	pj_ntoh_assign(storage, storage_len, room_id_);
	pj_ntoh_assign(storage, storage_len, user_id_);
}

void DelUserScene::Maintain(shared_ptr<TcpParameter> ptr_tcp_param, AvsProxy *avs_proxy)
{
	RETURN_IF_FAIL(avs_proxy != nullptr);

	DelUserParameter *param = reinterpret_cast<DelUserParameter *>(ptr_tcp_param.get());

	pj_status_t status;
	TitleRoom *title_room = nullptr;
	status = avs_proxy->GetRoom(param->room_id_, title_room);
	RETURN_IF_FAIL(status == PJ_SUCCESS && title_room != nullptr);

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

	title_room->DelUser(param->user_id_); // 然后删除这个用户
}
