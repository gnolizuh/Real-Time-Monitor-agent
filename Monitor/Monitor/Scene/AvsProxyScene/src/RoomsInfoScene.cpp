#include "stdafx.h"
#include "RoomsInfoScene.h"

RoomsInfoParameter::RoomsInfoParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: TcpParameter(storage, storage_len)
{
	pj_ntoh_assign(storage, storage_len, room_count_);
	for(pj_uint32_t i = 0; i < room_count_; ++ i)
	{
		rooms_info_.push_back(room_info_t());
		pj_ntoh_assign(storage, storage_len, rooms_info_[i].room_id_);
		pj_ntoh_assign(storage, storage_len, rooms_info_[i].user_count_);
		for(pj_uint8_t j = 0; j < rooms_info_[i].user_count_; ++ j)
		{
			rooms_info_[i].users_info_.push_back(user_info_t());
			pj_ntoh_assign(storage, storage_len, rooms_info_[i].users_info_[j].user_id_);
			pj_ntoh_assign(storage, storage_len, rooms_info_[i].users_info_[j].mic_id_);
			pj_ntoh_assign(storage, storage_len, rooms_info_[i].users_info_[j].audio_ssrc_);
			pj_ntoh_assign(storage, storage_len, rooms_info_[i].users_info_[j].video_ssrc_);
		}
	}
}

void RoomsInfoScene::Maintain(shared_ptr<TcpParameter> ptr_tcp_param, AvsProxy *avs_proxy)
{
	RoomsInfoParameter *param = reinterpret_cast<RoomsInfoParameter *>(ptr_tcp_param.get());
	RETURN_IF_FAIL(param->room_count_ > 0);

	TitleRoom *title_room = nullptr;
	for(pj_uint8_t i = 0; i < param->room_count_; ++ i)
	{
		title_room = nullptr;
		if(avs_proxy->GetRoom(param->rooms_info_[i].room_id_, title_room) == PJ_SUCCESS)
		{
			vector<User *> users;
			for(pj_uint8_t j = 0; j < param->rooms_info_[i].user_count_; ++ j)
			{
				pj_uint64_t user_id = param->rooms_info_[i].users_info_[j].user_id_;
				pj_uint32_t audio_ssrc = param->rooms_info_[i].users_info_[j].audio_ssrc_;
				pj_uint32_t video_ssrc = param->rooms_info_[i].users_info_[j].video_ssrc_;
				pj_uint32_t mic_id = param->rooms_info_[i].users_info_[j].mic_id_;
				User *user = title_room->AddUser(user_id, mic_id);
				title_room->ModUser(user, audio_ssrc, video_ssrc);
				users.push_back(user);
			}
			g_watchs_list.AddRoom(title_room);
		}
	}
}
