#include "stdafx.h"
#include "DelUserScene.h"

DelUserParameter::DelUserParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: TcpParameter(storage, storage_len)
{
	pj_ntoh_assign(storage, storage_len, room_id_);
	pj_ntoh_assign(storage, storage_len, user_id_);
}

void DelUserScene::Maintain(TcpParameter *parameter, RoomTreeCtl *room_ctl)
{
	DelUserParameter *param = reinterpret_cast<DelUserParameter *>(parameter);

	Room *room = room_ctl->GetRoom(param->room_id_);
	RETURN_IF_FAIL(room != nullptr);

	room_ctl->DelUser(room, param->user_id_);
}
