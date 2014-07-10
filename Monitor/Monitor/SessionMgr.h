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
	static pj_bool_t   LogRxDelivery(pjsip_rx_data *);
	static pj_status_t LogTxDelivery(pjsip_tx_data *);
	static void        LogPerform(int, const char *, int);
	static void        OnStateChanged(pjsip_inv_session *, pjsip_event *);
	static void        OnNewSession(pjsip_inv_session *, pjsip_event *);
	static void        OnMediaUpdate(pjsip_inv_session *, pj_status_t);
	
private:
	SessionMgr();
	~SessionMgr();
	static int SipThread(void *);

private:
	pj_caching_pool     caching_pool;
	pj_pool_t		   *pool;
	
	pj_thread_t        *sip_thread[1];
	pjsip_endpoint	   *sip_endpt;
	pjmedia_endpt      *media_endpt;
	pj_str_t		    local_addr;
	pj_str_t            local_uri;
	pj_uint16_t         sip_port;
	pj_uint16_t         media_port;
	vector<Session *>   sessions;
	static pj_bool_t    quit_flag;
	static pj_bool_t    call_report;
	static pj_oshandle_t log_handle;
	static mutex        g_instance_mutex;
	static pj_str_t     POOL_NAME;
	static pj_str_t     LOG_NAME;
	static SessionMgr    *instance;
	static pjsip_module monitor_module;
	static pjsip_module logger_module;
};

#endif
