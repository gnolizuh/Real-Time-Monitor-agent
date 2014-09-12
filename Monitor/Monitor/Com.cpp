#include "stdafx.h"
#include "Com.h"

pj_status_t pj_open_tcp_serverport(pj_str_t *ip, pj_uint16_t port, pj_sock_t &sock)
{
	pj_status_t status;
	status = pj_sock_socket(pj_AF_INET(), pj_SOCK_STREAM(), 0, &sock);
	RETURN_VAL_IF_FAIL( status == PJ_SUCCESS, status );

	int enabled = 1;
	status = pj_sock_setsockopt(sock, pj_SOL_SOCKET(), pj_SO_REUSEADDR(), &enabled, sizeof(enabled));
	RETURN_VAL_IF_FAIL( status == PJ_SUCCESS, (pj_sock_close(sock), status) );

	pj_sockaddr_in addr;
	status = pj_sockaddr_in_init(&addr, ip, port);
	RETURN_VAL_IF_FAIL( status == PJ_SUCCESS, (pj_sock_close(sock), status) );

	status = pj_sock_bind(sock, &addr, pj_sockaddr_get_len(&addr));
	RETURN_VAL_IF_FAIL( status == PJ_SUCCESS, (pj_sock_close(sock), status) );

	status = pj_sock_listen(sock, 5);
	RETURN_VAL_IF_FAIL( status == PJ_SUCCESS, (pj_sock_close(sock), status) );

	u_long val = 1;
#if defined(PJ_WIN32) && PJ_WIN32!=0 || \
    defined(PJ_WIN64) && PJ_WIN64 != 0 || \
    defined(PJ_WIN32_WINCE) && PJ_WIN32_WINCE!=0
    if (ioctlsocket(sock, FIONBIO, &val)) {
#else
    if (ioctl(new_sock, FIONBIO, &val)) {
#endif
        pj_sock_close(sock);
		return -1;
    }

	return status;
}

pj_status_t pj_open_tcp_clientport(pj_str_t *ip, pj_uint16_t port, pj_sock_t &sock)
{
	pj_status_t status;
	status = pj_sock_socket(pj_AF_INET(), pj_SOCK_STREAM(), 0, &sock);
	RETURN_VAL_IF_FAIL( status == PJ_SUCCESS, status );

	pj_sockaddr_in addr;
	status = pj_sockaddr_in_init(&addr, ip, port);
	RETURN_VAL_IF_FAIL( status == PJ_SUCCESS, (pj_sock_close(sock), status) );

	status = pj_sock_connect(sock, &addr, sizeof(addr));
	RETURN_VAL_IF_FAIL( status == PJ_SUCCESS, (pj_sock_close(sock), status) );

	u_long val = 1;
#if defined(PJ_WIN32) && PJ_WIN32!=0 || \
    defined(PJ_WIN64) && PJ_WIN64 != 0 || \
    defined(PJ_WIN32_WINCE) && PJ_WIN32_WINCE!=0
    if (ioctlsocket(sock, FIONBIO, &val)) {
#else
    if (ioctl(new_sock, FIONBIO, &val)) {
#endif
        pj_sock_close(sock);
		return -1;
    }

	return status;
}

pj_status_t pj_open_udp_transport(pj_str_t *ip, pj_uint16_t port, pj_sock_t &sock)
{
	pj_status_t status;
	status = pj_sock_socket(pj_AF_INET(), pj_SOCK_DGRAM(), 0, &sock);
	RETURN_VAL_IF_FAIL( status == PJ_SUCCESS, status );

	int enabled = 1;
	status = pj_sock_setsockopt(sock, pj_SOL_SOCKET(), pj_SO_REUSEADDR(), &enabled, sizeof(enabled));
	RETURN_VAL_IF_FAIL( status == PJ_SUCCESS, (pj_sock_close(sock), status) );

	if ( ip != nullptr && port > 0 )
	{
		pj_sockaddr_in addr;
		status = pj_sockaddr_in_init(&addr, ip, port);
		RETURN_VAL_IF_FAIL( status == PJ_SUCCESS, (pj_sock_close(sock), status) );

		status = pj_sock_bind(sock, &addr, pj_sockaddr_get_len(&addr));
		RETURN_VAL_IF_FAIL( status == PJ_SUCCESS, (pj_sock_close(sock), status) );
	}

	u_long val = 1;
#if defined(PJ_WIN32) && PJ_WIN32!=0 || \
    defined(PJ_WIN64) && PJ_WIN64 != 0 || \
    defined(PJ_WIN32_WINCE) && PJ_WIN32_WINCE!=0
    if (ioctlsocket(sock, FIONBIO, &val)) {
#else
    if (ioctl(new_sock, FIONBIO, &val)) {
#endif
        pj_sock_close(sock);
		return -1;
    }

	return status;
}

pj_uint64_t pj_ntohll(pj_uint64_t netlonglong)
{
	return ntohll(netlonglong);
}

pj_uint64_t pj_htonll(pj_uint64_t hostlonglong)
{
	return htonll(hostlonglong);
}

static pj_oshandle_t g_log_handle;
pj_status_t log_open(pj_pool_t *pool, const pj_str_t &file_name)
{
	pj_log_set_log_func(log_writer);

	return pj_file_open(pool, file_name.ptr, PJ_O_WRONLY | PJ_O_APPEND, &g_log_handle);
}

void log_writer(int level, const char *log, int loglen)
{
	pj_ssize_t log_size = loglen;
	pj_file_write(g_log_handle, log, &log_size);
	pj_file_flush(g_log_handle);
}

static void OnData( const happyhttp::Response* r, void* userdata, const unsigned char* data, int n )
{
	std::vector<pj_uint8_t> &response = *(reinterpret_cast<std::vector<pj_uint8_t> *>(userdata));
	response.insert(response.end(), data, data + n);
}

void http_tls_get(const pj_str_t &host, pj_uint16_t port, const pj_str_t &url,
				  pj_uint32_t node_id, std::vector<pj_uint8_t> &response)
{
#define ATTR_NODE_ID "&node_id="
	std::stringstream ss_uri;
	ss_uri << url.ptr << ATTR_NODE_ID << node_id;

	happyhttp::Connection conn(host.ptr, port);
	conn.setcallbacks(0, OnData, 0, &response);
	conn.request("GET", ss_uri.str().c_str(), 0, 0, 0);

	while( conn.outstanding() )
		conn.pump();
}

void http_proxy_get(const pj_str_t &host, pj_uint16_t port, const pj_str_t &url,
					pj_uint32_t room_id, std::vector<pj_uint8_t> &response)
{
#define ATTR_ROOM_ID "roomid="
	std::stringstream ss_uri;
	ss_uri << url.ptr << ATTR_ROOM_ID << room_id;

	happyhttp::Connection conn(host.ptr, port);
	conn.setcallbacks(0, OnData, 0, &response);
	conn.request("GET", ss_uri.str().c_str(), 0, 0, 0);

	while( conn.outstanding() )
		conn.pump();
}

pj_status_t UTF8_to_GB2312(wchar_t *gb_dst, int gb_len, const pj_str_t &utf_src)
{
	int res = MultiByteToWideChar(CP_UTF8, 0, utf_src.ptr, utf_src.slen, gb_dst, gb_len);
	return res != 0 ? PJ_SUCCESS : PJ_EINVAL;
}