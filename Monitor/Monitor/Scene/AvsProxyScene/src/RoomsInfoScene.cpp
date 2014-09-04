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
			pj_ntoh_assign(storage, storage_len, rooms_info_[i].users_info_[j].audio_ssrc_);
			pj_ntoh_assign(storage, storage_len, rooms_info_[i].users_info_[j].video_ssrc_);
		}
	}
}

void RoomsInfoScene::Maintain(shared_ptr<TcpParameter> ptr_tcp_param, Node *node)
{
	RoomsInfoParameter *param = reinterpret_cast<RoomsInfoParameter *>(ptr_tcp_param.get());
	for(pj_uint32_t i = 0; i < param->room_count_; ++ i)
	{
		/*Room *room = room_ctl->AddRoom(param->rooms_info_[i].room_id_);
		for(pj_uint8_t j = 0; j < param->rooms_info_[i].user_count_; ++ j)
		{
			User *user = room_ctl->AddUser(room, param->rooms_info_[i].users_info_[j].user_id_);
			room_ctl->ModMedia(user, param->rooms_info_[i].users_info_[j].audio_ssrc_,
				param->rooms_info_[i].users_info_[j].video_ssrc_);
		}*/
	}
}
