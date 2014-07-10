#ifndef __MONITOR_UTILPACKET__
#define __MONITOR_UTILPACKET__

#include "common.h"

namespace sinashow
{

typedef enum
{
	UTIL_PACKET_SIP,
	UTIL_PACKET_MEDIA,
} util_packet_type_t;

typedef enum
{
	UTIL_SIP_PACKET_CONNECTING,
	UTIL_SIP_PACKET_CONNECTED,
	UTIL_SIP_PACKET_DISCONNECTED
} util_sip_packet_type_t;

typedef struct
{
	util_packet_type_t type;
	void              *buf;
	pj_uint32_t        buflen;
} util_packet_t;

};

#endif
