#include "stdafx.h"
#include "DelUserScene.h"

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

	users_map_t::iterator puser;
	title_room->DelUser(param->user_id_, puser);
}
