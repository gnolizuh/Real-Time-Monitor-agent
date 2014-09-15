#include "stdafx.h"
#include "ResLoginScene.h"

ResLoginParameter::ResLoginParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: TcpParameter(storage, storage_len)
{
}

void ResLoginScene::Maintain(shared_ptr<TcpParameter> ptr_tcp_param, AvsProxy *avs_proxy)
{
	RETURN_IF_FAIL(avs_proxy != nullptr);

	ResLoginParameter *param = reinterpret_cast<ResLoginParameter *>(ptr_tcp_param.get());

	avs_proxy->OnRxLogin();
}
