#include "stdafx.h"
#include "AddUserScene.h"

AddUserParameter::AddUserParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: TcpParameter(storage, storage_len)
{
	pj_ntoh_assign(storage, storage_len, room_id_);
	pj_ntoh_assign(storage, storage_len, user_id_);
}

void AddUserScene::Maintain(shared_ptr<TcpParameter> ptr_tcp_param, Node *node)
{
	AddUserParameter *param = reinterpret_cast<AddUserParameter *>(ptr_tcp_param.get());

	/*Room *room = room_ctl->GetRoom(param->room_id_);
	RETURN_IF_FAIL(room != nullptr);

	room_ctl->AddUser(room, param->user_id_);*/
}
