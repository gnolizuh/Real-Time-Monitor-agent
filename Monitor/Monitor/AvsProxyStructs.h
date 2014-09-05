#ifndef __AVS_PROXY_CLIENT_AVS_PROXY_STRUCTS__
#define __AVS_PROXY_CLIENT_AVS_PROXY_STRUCTS__

#include "Com.h"

#pragma pack(1)

typedef struct
{
	void Serialize()
	{
		length = serialize((pj_uint16_t)(sizeof(request_to_avs_proxy_login_t) - sizeof(length)));
		client_request_type = serialize(client_request_type);
		proxy_id = serialize(proxy_id);
		client_id = serialize(client_id);
		media_port = serialize(media_port);
	}

private:
	pj_uint16_t length;

public:
	pj_uint16_t client_request_type;
	pj_uint16_t proxy_id;
	pj_uint16_t client_id;
	pj_in_addr  media_ip;      // MUST manual convert media_ip from host to network.
	pj_uint16_t media_port;
} request_to_avs_proxy_login_t;

typedef struct
{
	void Serialize()
	{
		length = serialize((pj_uint16_t)(sizeof(request_to_avs_proxy_logout_t) - sizeof(length)));
		client_request_type = serialize(client_request_type);
		proxy_id = serialize(proxy_id);
		client_id = serialize(client_id);
	}

private:
	pj_uint16_t length;

public:
	pj_uint16_t client_request_type;
	pj_uint16_t proxy_id;
	pj_uint16_t client_id;
} request_to_avs_proxy_logout_t;

typedef struct
{
	void Serialize()
	{
		length = serialize((pj_uint16_t)(sizeof(request_to_avs_proxy_logout_t) - sizeof(length)));
		client_request_type = serialize(client_request_type);
		proxy_id = serialize(proxy_id);
		client_id = serialize(client_id);
	}

private:
	pj_uint16_t length;

public:
	pj_uint16_t client_request_type;
	pj_uint16_t proxy_id;
	pj_uint16_t client_id;
	pj_int32_t  room_id;
} request_to_avs_proxy_link_room_t;

typedef request_to_avs_proxy_link_room_t request_to_avs_proxy_unlink_room_t;

typedef struct
{
	void Serialize()
	{
		length = serialize((pj_uint16_t)(sizeof(request_to_avs_proxy_link_room_user_t) - sizeof(length)));
		client_request_type = serialize(client_request_type);
		proxy_id = serialize(proxy_id);
		client_id = serialize(client_id);
		room_id = serialize(room_id);
		user_id = serialize(user_id);
		link_media_mask = serialize(link_media_mask);
	}

private:
	pj_uint16_t length;

public:
	pj_uint16_t client_request_type;
	pj_uint16_t proxy_id;
	pj_uint16_t client_id;
	pj_int32_t  room_id;
	pj_int64_t  user_id;
	pj_uint8_t  link_media_mask;
} request_to_avs_proxy_link_room_user_t;

typedef struct
{
	void Serialize()
	{
		length = serialize((pj_uint16_t)(sizeof(request_to_avs_proxy_unlink_room_user_t) - sizeof(length)));
		client_request_type = serialize(client_request_type);
		proxy_id = serialize(proxy_id);
		client_id = serialize(client_id);
		room_id = serialize(room_id);
		user_id = serialize(user_id);
		unlink_media_mask = serialize(unlink_media_mask);
	}

private:
	pj_uint16_t length;

public:
	pj_uint16_t client_request_type;
	pj_uint16_t proxy_id;
	pj_uint16_t client_id;
	pj_int32_t  room_id;
	pj_int64_t  user_id;
	pj_uint8_t  unlink_media_mask;
} request_to_avs_proxy_unlink_room_user_t;

typedef struct
{
	void Serialize()
	{
		length = serialize((pj_uint16_t)(sizeof(request_to_avs_proxy_keep_alive_t) - sizeof(length)));
		client_request_type = serialize(client_request_type);
		proxy_id = serialize(proxy_id);
		client_id = serialize(client_id);
	}

private:
	pj_uint16_t length;

public:
	pj_uint16_t client_request_type;
	pj_uint16_t proxy_id;
	pj_uint16_t client_id;
} request_to_avs_proxy_keep_alive_t;

#pragma pack()

#endif
