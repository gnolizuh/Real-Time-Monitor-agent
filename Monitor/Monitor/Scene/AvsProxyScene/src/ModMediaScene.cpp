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

void ModMediaScene::Maintain(shared_ptr<TcpParameter> ptr_tcp_param, AvsProxy *avs_proxy)
{
	RETURN_IF_FAIL(avs_proxy != nullptr);

	ModMediaParameter *param = reinterpret_cast<ModMediaParameter *>(ptr_tcp_param.get());

	pj_status_t status;
	TitleRoom *title_room = nullptr;
	status = avs_proxy->GetRoom(param->room_id_, title_room);
	RETURN_IF_FAIL(status == PJ_SUCCESS);

	User *user = title_room->GetUser(param->user_id_);
	RETURN_IF_FAIL(user != nullptr);

	title_room->ModUser(user, param->audio_ssrc_, param->video_ssrc_);
}
