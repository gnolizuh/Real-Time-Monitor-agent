#include "stdafx.h"
#include "NATScene.h"

NATParameter::NATParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: UdpParameter(storage, storage_len)
{
	pj_ntoh_assign(storage, storage_len, client_id_);
}

void NATScene::Maintain(shared_ptr<UdpParameter> ptr_udp_param, AvsProxy *avs_proxy)
{
	RETURN_IF_FAIL(avs_proxy != nullptr);

	NATParameter *param = reinterpret_cast<NATParameter *>(ptr_udp_param.get());

	avs_proxy->OnRxNAT();
}
