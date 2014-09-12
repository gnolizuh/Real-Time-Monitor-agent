#include "stdafx.h"
#include "AddUserScene.h"

AddUserParameter::AddUserParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: TcpParameter(storage, storage_len)
{
	pj_ntoh_assign(storage, storage_len, room_id_);
	pj_ntoh_assign(storage, storage_len, user_id_);
}

void AddUserScene::Maintain(shared_ptr<TcpParameter> ptr_tcp_param, AvsProxy *avs_proxy)
{
	RETURN_IF_FAIL(avs_proxy != nullptr);

	AddUserParameter *param = reinterpret_cast<AddUserParameter *>(ptr_tcp_param.get());

	pj_status_t status;
	TitleRoom *title_room = nullptr;
	status = avs_proxy->GetRoom(param->room_id_, title_room);
	RETURN_IF_FAIL(status == PJ_SUCCESS);

	title_room->AddUser(param->user_id_);
}
