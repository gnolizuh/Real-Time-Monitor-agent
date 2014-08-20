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
pj_status_t log_open(pj_pool_t *pool, pj_str_t file_name)
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
