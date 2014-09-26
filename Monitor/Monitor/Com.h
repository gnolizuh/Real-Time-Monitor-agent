#ifndef __AVS_PROXY_CLIENT_COM__
#define __AVS_PROXY_CLIENT_COM__

#include <event.h>
#include <memory>
#include <functional>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <set>

#include <pjlib.h>
#include <pjmedia.h>

#include <SDL.h>
#include <SDL_thread.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
}

#include "happyhttp.h"
#include "command.h"

using std::map;
using std::set;
using std::vector;

typedef uint32_t room_id_t;
typedef std::function<void (intptr_t, short, void *)> ev_function_t;
typedef map<pj_uint32_t, pj_uint8_t> index_map_t;
typedef vector<index_map_t> av_index_map_t;
typedef pj_uint32_t order_t;

#define INVALID_SCREEN_INDEX      -1
#define MAXIMAL_SCREEN_NUM         15
#define MAXIMAL_THREAD_NUM         1
#define MAX_STORAGE_SIZE           1024
#define MAX_TRANSMISSION_UNIT_SIZE 1500
#define IP_HEADER_SIZE             20
#define UDP_HEADER_SIZE            8
#define MAX_UDP_DATA_SIZE (MAX_TRANSMISSION_UNIT_SIZE - IP_HEADER_SIZE - UDP_HEADER_SIZE)
#define MIN(m1, m2) ((m1) < (m2) ? (m1) : (m2))
#define MAX(m1, m2) ((m1) > (m2) ? (m1) : (m2))
enum { AUDIO_INDEX, VIDEO_INDEX };

extern evutil_socket_t g_mainframe_pipe[2];
extern index_map_t g_av_index_map[2];

typedef enum __enum_screen_mgr_resolution_type__
{
	SCREEN_RES_1x1,
	SCREEN_RES_2x2,
	SCREEN_RES_1x5,
	SCREEN_RES_3x3,
	SCREEN_RES_3x5,
	SCREEN_RES_END,            // useless
} enum_screen_mgr_resolution_t;

typedef struct
{
	pj_uint32_t Msg;
	WPARAM      wParam;
	LPARAM      lParam;
} param_t;

#define RETURN_VAL_IF_FAIL(_macro_exp_, _macro_ret_) do { \
	if ( !(_macro_exp_) ) return (_macro_ret_); \
} while(0)

#define RETURN_IF_FAIL(_macro_exp_) do { \
	if ( !(_macro_exp_) ) return; \
} while(0)

#define RETURN_VAL_WITH_STATEMENT_IF_FAIL(_macro_exp_, _statement_, _macro_ret_) do { \
	if ( !(_macro_exp_) ) { (_statement_); return (_macro_ret_); }\
} while(0)

#define RETURN_WITH_STATEMENT_IF_FAIL(_macro_exp_, _statement_) do { \
	if ( !(_macro_exp_) ) { (_statement_); return; }\
} while(0)

class Noncopyable
{
public:
	Noncopyable() {}

private:
	Noncopyable(const Noncopyable &) {}
	Noncopyable &operator=(const Noncopyable &) {}
};

pj_status_t pj_open_tcp_serverport(pj_str_t *ip, pj_uint16_t port, pj_sock_t &sock);
pj_status_t pj_open_tcp_clientport(pj_str_t *ip, pj_uint16_t port, pj_sock_t &sock);
pj_status_t pj_open_udp_transport(pj_str_t *ip, pj_uint16_t port, pj_sock_t &sock);

/**
 * Convert 64-bit value from network byte order to host byte order.
 *
 * @param netlonglong   64-bit network value.
 * @return	            64-bit host value.
 */
pj_uint64_t pj_ntohll(pj_uint64_t netlonglong);

/**
 * Convert 64-bit value from host byte order to network byte order.
 *
 * @param hostlonglong  64-bit host value.
 * @return	            64-bit network value.
 */
pj_uint64_t pj_htonll(pj_uint64_t hostlonglong);


template<typename Type>
inline Type serialize(Type t)
{
	if ( sizeof(t) == sizeof(pj_uint8_t) )
	{
		return t;
	}
	else if ( sizeof(t) == sizeof(pj_uint16_t) )
	{
		return (Type)pj_htons((pj_uint16_t)t);
	}
	else if ( sizeof(t) == sizeof(pj_uint32_t) )
	{
		return (Type)pj_htonl((pj_uint32_t)t);
	}
	else if ( sizeof(t) == sizeof(pj_uint64_t) )
	{
		return (Type)pj_htonll((pj_uint64_t)t);
	}
	else
	{
		pj_assert(!"Don't serialize a number which value is more then 64bit!!");
		return (Type)0;
	}
}

/**
 * Convert value from network byte order to host byte order arbitrarily. 
 *
 * @param t network value.
 */
template<typename Type>
inline Type unserialize(Type t)
{
	if ( sizeof(t) == sizeof(uint8_t) )
	{
		return t;
	}
	else if ( sizeof(t) == sizeof(uint16_t) )
	{
		return (Type)pj_ntohs((uint16_t)t);
	}
	else if ( sizeof(t) == sizeof(uint32_t) )
	{
		return (Type)pj_ntohl((uint32_t)t);
	}
	else if ( sizeof(t) == sizeof(uint64_t) )
	{
		return (Type)pj_ntohll((uint64_t)t);
	}
	else
	{
		assert(!"Don't unserialize a number which value is more then 64bit!!");
		return (Type)0;
	}
}

template<typename T>
void pj_ntoh_assign(const pj_uint8_t *&storage, pj_uint16_t &storage_len, T &rval)
{
	rval = unserialize(*(T *)storage);
	storage += sizeof(T);
	storage_len -= sizeof(T);
}

template<class T>
struct order_cmp
{
	bool operator() (const T *t1, const T *t2)
	{
		return t1->order_ < t2->order_;
	}
};

pj_status_t log_open(pj_pool_t *pool, const pj_str_t &file_name);
void        log_writer(int level, const char *log, int loglen);

void        http_tls_get(const pj_str_t &host, pj_uint16_t port, const pj_str_t &url, pj_uint32_t node_id, std::vector<pj_uint8_t> &response);
void        http_proxy_get(const pj_str_t &host, pj_uint16_t port, const pj_str_t &url, pj_uint32_t room_id, std::vector<pj_uint8_t> &response);

pj_status_t UTF8_to_GB2312(wchar_t *gb_dst, int gb_len, const pj_str_t &utf_src);

namespace sinashow
{
void SendMessage(pj_uint32_t Msg, WPARAM wParam, LPARAM lParam);
}

#endif
