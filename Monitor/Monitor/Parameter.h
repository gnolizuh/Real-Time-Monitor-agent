#ifndef __AVS_PROXY_CLIENT_PARAMETER__
#define __AVS_PROXY_CLIENT_PARAMETER__

#include "Com.h"

#pragma pack(1)
class TcpParameter
{
public:
	TcpParameter(const pj_uint8_t *&storage, pj_uint16_t &storage_len)
	{
		pj_ntoh_assign(storage, storage_len, length_);
		pj_ntoh_assign(storage, storage_len, client_request_type_);
		pj_ntoh_assign(storage, storage_len, proxy_id_);
		pj_ntoh_assign(storage, storage_len, client_id_);
	}

	pj_uint16_t length_;
	pj_uint16_t client_request_type_;
	pj_uint16_t proxy_id_;
	pj_uint16_t client_id_;
};
#pragma pack()

class UdpParameter
{
};

#endif
