#include "stdafx.h"
#include "ModMediaScene.h"

ModMediaParameter::ModMediaParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: TcpParameter(storage, storage_len)
{
	pj_ntoh_assign(storage, storage_len, room_id_);
	pj_ntoh_assign(storage, storage_len, user_id_);
	pj_ntoh_assign(storage, storage_len, audio_ssrc_);
	pj_ntoh_assign(storage, storage_len, video_ssrc_);
}

void ModMediaScene::Maintain(shared_ptr<TcpParameter> ptr_tcp_param, Node *node)
{
	ModMediaParameter *param = reinterpret_cast<ModMediaParameter *>(ptr_tcp_param.get());

	/*Room *room = room_ctl->GetRoom(param->room_id_);
	RETURN_IF_FAIL(room != nullptr);

	User *user = room_ctl->GetUser(room, param->user_id_);
	RETURN_IF_FAIL(user != nullptr);

	room_ctl->ModMedia(user, param->audio_ssrc_, param->video_ssrc_);*/
}
