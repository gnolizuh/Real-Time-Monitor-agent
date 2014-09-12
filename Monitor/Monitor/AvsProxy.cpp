#include "stdafx.h"
#include "AvsProxy.h"

extern Config g_client_config;

AvsProxy::AvsProxy(pj_uint16_t id, const pj_str_t &ip, pj_uint16_t tcp_port, pj_uint16_t udp_port, pj_sock_t sock)
	: pfunction_(nullptr)
	, tcp_ev_(nullptr)
	, sock_(sock)
	, status_(AVS_PROXY_STATUS_OFFLINE)
	, id_(id)
	, ip_(pj_str(strdup(ip.ptr)))
	, tcp_port_(tcp_port)
	, udp_port_(udp_port)
	, active_(PJ_FALSE)
	, tcp_storage_offset_(0)
{
}

pj_status_t AvsProxy::Login()
{
	RETURN_VAL_IF_FAIL(status_ == AVS_PROXY_STATUS_OFFLINE, PJ_SUCCESS);

	request_to_avs_proxy_login_t login;
	login.client_request_type = REQUEST_FROM_CLIENT_TO_AVSPROXY_LOGIN;
	login.proxy_id = id_;
	login.client_id = g_client_config.client_id;
	pj_inet_aton(&g_client_config.local_ip, &login.media_ip);
	login.media_port = g_client_config.local_media_port;
	login.Serialize();

	pj_ssize_t sndlen = sizeof(login);
	pj_status_t status = SendTCPPacket(&login, &sndlen);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	status_ = AVS_PROXY_STATUS_LOGINING;
	status_ = AVS_PROXY_STATUS_ONLINE;

	return PJ_SUCCESS;
}

pj_status_t AvsProxy::LinkRoomUser(TitleRoom *title_room, User *user)
{
	enum {AUDIO_INDEX, VIDEO_INDEX};

	RETURN_VAL_IF_FAIL(user, PJ_EINVAL);

	request_to_avs_proxy_link_room_user_t link_room_user;
	link_room_user.client_request_type = REQUEST_FROM_CLIENT_TO_AVSPROXY_LINK_ROOM_USER;
	link_room_user.proxy_id = id_;
	link_room_user.client_id = g_client_config.client_id;
	link_room_user.room_id = user->room_id_;
	link_room_user.user_id = user->user_id_;
	link_room_user.link_media_mask = MEDIA_MASK_VIDEO;
	link_room_user.Serialize();

	pj_ssize_t sndlen = sizeof(link_room_user);
	return SendTCPPacket(&link_room_user, &sndlen);
}

pj_status_t AvsProxy::UnlinkRoomUser(TitleRoom *title_room, User *user)
{
	enum {AUDIO_INDEX, VIDEO_INDEX};

	RETURN_VAL_IF_FAIL(user, PJ_EINVAL);

	request_to_avs_proxy_unlink_room_user_t unlink_room_user;
	unlink_room_user.client_request_type = REQUEST_FROM_CLIENT_TO_AVSPROXY_UNLINK_ROOM_USER;
	unlink_room_user.proxy_id = id_;
	unlink_room_user.client_id = g_client_config.client_id;
	unlink_room_user.room_id = user->room_id_;
	unlink_room_user.user_id = user->user_id_;
	unlink_room_user.unlink_media_mask = MEDIA_MASK_VIDEO;
	unlink_room_user.Serialize();

	pj_ssize_t sndlen = sizeof(unlink_room_user);
	return SendTCPPacket(&unlink_room_user, &sndlen);
}

pj_status_t AvsProxy::LinkRoom(TitleRoom *title_room)
{
	RETURN_VAL_IF_FAIL(AddRoom(title_room->id_, title_room) == PJ_SUCCESS, PJ_EEXISTS);

	request_to_avs_proxy_link_room_t link_room;
	link_room.client_request_type = REQUEST_FROM_CLIENT_TO_AVSPROXY_LINK_ROOM;
	link_room.proxy_id = id_;
	link_room.client_id = g_client_config.client_id;
	link_room.room_id = title_room->id_;
	link_room.Serialize();

	pj_ssize_t sndlen = sizeof(link_room);
	return SendTCPPacket(&link_room, &sndlen);
}

pj_status_t AvsProxy::AddRoom(pj_int32_t room_id, TitleRoom *title_room)
{
	lock_guard<mutex> lock(rooms_lock_);
	room_map_t::iterator proom = rooms_.find(room_id);
	RETURN_VAL_IF_FAIL(proom == rooms_.end(), PJ_EEXISTS);

	rooms_.insert(room_map_t::value_type(room_id, title_room));
	return PJ_SUCCESS;
}

pj_status_t AvsProxy::GetRoom(pj_int32_t room_id, TitleRoom *&title_room)
{
	lock_guard<mutex> lock(rooms_lock_);
	room_map_t::iterator proom = rooms_.find(room_id);
	RETURN_VAL_IF_FAIL(proom != rooms_.end(), PJ_EINVAL);

	title_room = proom->second;
	RETURN_VAL_IF_FAIL(title_room != nullptr, PJ_EINVAL);

	return PJ_SUCCESS;
}

pj_status_t AvsProxy::SendTCPPacket(const void *buf, pj_ssize_t *len)
{
	lock_guard<mutex> lock(tcp_lock_);
	return pj_sock_send(sock_, buf, len, 0);
}
