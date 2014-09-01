#include "stdafx.h"
#include "KeepAliveScene.h"

KeepAliveParameter::KeepAliveParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: TcpParameter(storage, storage_len)
{
}

void KeepAliveScene::Maintain(shared_ptr<TcpParameter> ptr_tcp_param)
{
	KeepAliveParameter *param = reinterpret_cast<KeepAliveParameter *>(ptr_tcp_param.get());

}
