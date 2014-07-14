#ifndef __MONITOR_SESSION_MGR__
#define __MONITOR_SESSION_MGR__

#include <thread>
#include <vector>
#include <mutex>
#include "ScreenMgr.h"
#include "Session.h"
#include "common.h"

using std::lock_guard;
using std::vector;
using std::thread;
using std::mutex;

class Session;

class SessionMgr
{
public:
	static SessionMgr *GetInstance();                // Singleton
	pj_status_t Prepare(pj_str_t, pj_uint16_t, pj_uint16_t);
	void Launch();
	pj_status_t StartSession(const pj_str_t *, pj_uint8_t);
	pj_status_t StopSession(pj_uint8_t);

protected:
	SessionMgr();
	~SessionMgr();
	static int         SipThread(void *);
	static pj_bool_t   LogRxDelivery(pjsip_rx_data *);
	static pj_status_t LogTxDelivery(pjsip_tx_data *);
	static void        LogPerform(int, const char *, int);
	static void        OnStateChanged(pjsip_inv_session *, pjsip_event *);
	static void        OnNewSession(pjsip_inv_session *, pjsip_event *);
	static void        OnMediaUpdate(pjsip_inv_session *, pj_status_t);
	
private:
	inline pj_caching_pool &caching_pool() { return caching_pool_; }
	inline pj_pool_t *pool() const { return pool_; }
	inline pjsip_endpoint *sip_endpt() const { return sip_endpt_; }
	inline pjmedia_endpt *media_endpt() const { return media_endpt_; }
	inline pj_str_t &local_sip_addr() { return local_sip_addr_; }
	inline pj_str_t &local_sip_uri() { return local_sip_uri_; }
	inline pj_uint16_t &local_sip_port() { return local_sip_port_; }
	inline pj_uint16_t &local_media_port() { return local_media_port_; }
	inline vector<Session *> &sessions() { return sessions_; }
	inline vector<pj_thread_t *> &sip_threads() { return sip_threads_; }

	inline void set_caching_pool(pj_caching_pool caching_pool) { caching_pool_ = caching_pool; }
	inline void set_pool(pj_pool_t *pool) { pool_ = pool; }
	inline void set_sip_endpt(pjsip_endpoint *sip_endpt) { sip_endpt_ = sip_endpt; }
	inline void set_media_endpt(pjmedia_endpt *media_endpt) { media_endpt_ = media_endpt; }
	inline void set_local_sip_addr(pj_str_t &local_sip_addr) { local_sip_addr_ = local_sip_addr; }
	inline void set_local_sip_uri(pj_str_t &local_sip_uri) { local_sip_uri_ = local_sip_uri; }
	inline void set_local_sip_port(pj_uint16_t local_sip_port) { local_sip_port_ = local_sip_port; }
	inline void set_local_media_port(pj_uint16_t local_media_port) { local_media_port_ = local_media_port; }

private:
	const pj_uint8_t    sip_thread_cnt_;
	pj_caching_pool     caching_pool_;
	pj_pool_t		   *pool_;
	pjsip_endpoint	   *sip_endpt_;
	pjmedia_endpt      *media_endpt_;
	pj_str_t		    local_sip_addr_;
	pj_str_t            local_sip_uri_;
	pj_uint16_t         local_sip_port_;
	pj_uint16_t         local_media_port_;
	vector<Session *>   sessions_;
	vector<pj_thread_t *> sip_threads_;

	static pj_bool_t     g_quit_flag;
	static pj_bool_t     g_call_report;
	static pj_oshandle_t g_log_handle;
	static mutex         g_instance_mutex;
	static pj_str_t      POOL_NAME;
	static pj_str_t      LOG_NAME;
	static SessionMgr   *g_instance;
	static pjsip_module  g_monitor_module;
	static pjsip_module  g_logger_module;
};

#endif
