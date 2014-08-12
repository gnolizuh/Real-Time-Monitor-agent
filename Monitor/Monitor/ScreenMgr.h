#ifndef __MONITOR_SCREEN_MGR__
#define __MONITOR_SCREEN_MGR__

#include <vector>
#include <thread>
#include <mutex>
#include <map>

#include "MessageQueue.hpp"
#include "Resource.h"
#include "Parameter.h"
#include "Scene.h"
#include "Screen.h"
#include "RoomsInfoScene.h"
#include "ModMediaScene.h"
#include "AddUserScene.h"
#include "DelUserScene.h"
#include "RTPScene.h"

#define TOP_SIDE_SIZE         30
#define SIDE_SIZE             8
#define MININUM_PADDING       0
#define MININUM_SCREEN_WIDTH  176
#define NIMINUM_SCREEN_HEIGHT 144
#define ROUND(num, fraction) (num) /= (fraction), (num) *= (fraction), (num) / (fraction)
#define GET_FUNC_INDEX(idx) ((idx) < 0 || (idx) >= SCREEN_RES_END ? SCREEN_RES_END : (idx))

typedef struct
{
	pj_uint32_t x;
	pj_uint32_t y;
} resolution_t;

using std::thread;
using std::mutex;
using std::lock_guard;
using std::vector;
using std::map;

class ScreenMgr;
typedef void (ScreenMgr::*screenmgr_func_t)(pj_uint32_t, pj_uint32_t);
typedef map<pj_uint32_t, pj_uint8_t> index_map_t;

class ScreenMgr
	: public Noncopyable
{
public:
	ScreenMgr(CWnd *, const pj_str_t &, pj_uint16_t, const pj_str_t &, pj_uint16_t);
	~ScreenMgr();

	pj_status_t Prepare(const pj_str_t &);
	pj_status_t Launch();
	void        Destory();
	void        Refresh(enum_screen_mgr_resolution_t);
	void        GetSuitedSize(LPRECT);
	void        Adjest(pj_int32_t &, pj_int32_t &);
	void        HideAll();
	static resolution_t GetDefaultResolution();

protected:
	static void event_on_tcp_read(evutil_socket_t, short, void *);
	static void event_on_udp_read(evutil_socket_t, short, void *);

	void EventOnTcpRead(evutil_socket_t, short);
	void EventOnUdpRead(evutil_socket_t, short);
	void ConnectorThread();
	void EventThread();

private:
	void TcpParamScene(const pj_uint8_t *, pj_uint16_t);
	void UdpParamScene(const pjmedia_rtp_hdr *, const pj_uint8_t *, pj_uint16_t);
	void Refresh_1x1(pj_uint32_t, pj_uint32_t);
	void Refresh_2x2(pj_uint32_t, pj_uint32_t);
	void Refresh_1x5(pj_uint32_t, pj_uint32_t);
	void Refresh_3x3(pj_uint32_t, pj_uint32_t);

private:
	const CWnd         *wrapper_;
	vector<Screen *>    screens_;
	vector<index_map_t> av_index_map_;
	pj_uint32_t         width_;
	pj_uint32_t         height_;
	pj_uint32_t         vertical_padding_;
	pj_uint32_t         horizontal_padding_;
	pj_sock_t           local_tcp_sock_;
	pj_sock_t           local_udp_sock_;
	pj_str_t		    avsproxy_ip_;
	pj_uint16_t         avsproxy_tcp_port_;
	pj_str_t		    local_ip_;
	pj_uint16_t         local_udp_port_;
	pj_caching_pool     caching_pool_;
	pj_pool_t		   *pool_;
	struct event       *tcp_ev_, *udp_ev_;
	struct event_base  *evbase_;
	pj_uint8_t          tcp_storage_[MAX_STORAGE_SIZE];
	pj_uint16_t         tcp_storage_offset_;
	thread              connector_thread_;
	thread              event_thread_;
	pj_bool_t           active_;
	vector<screenmgr_func_t> screenmgr_func_array_;
	vector<pj_uint32_t> num_blocks_;
	enum_screen_mgr_resolution_t screen_mgr_res_;

	static const resolution_t DEFAULT_RESOLUTION;
};

#endif
