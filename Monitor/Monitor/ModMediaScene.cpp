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

void ModMediaScene::Maintain(TcpParameter *parameter)
{
	ModMediaParameter *param = reinterpret_cast<ModMediaParameter *>(parameter);

}
