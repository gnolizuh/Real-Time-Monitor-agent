#ifndef __MONITOR_SESSION_MGR__
#define __MONITOR_SESSION_MGR__

#include <thread>
#include <mutex>
#include "Session.h"
#include "common.h"

using std::lock_guard;
using std::thread;
using std::mutex;

struct Dialog
{
	unsigned		     index;
    pjsip_inv_session	*inv;
    unsigned		     media_count;
    struct media_stream	 media[2];      /**< Support both audio and video. */
    pj_time_val		     start_time;
    pj_time_val		     response_time;
    pj_time_val		     connect_time;
    pj_timer_entry	     d_timer;	    /**< Disconnect timer.	*/
};

class SessionMgr
{
public:
	static SessionMgr *GetInstance();                // Singleton
	pj_status_t Prepare(pj_str_t, pj_uint16_t, pj_uint16_t);
	void Launch();
	pj_status_t StartSession(const pj_str_t *, pj_uint8_t);

protected:
	static pj_bool_t   LogRxDelivery(pjsip_rx_data *);
	static pj_status_t LogTxDelivery(pjsip_tx_data *);
	static void        LogPerform(int, const char *, int);
	static void        OnStateChanged(pjsip_inv_session *, pjsip_event *);
	static void        OnNewSession(pjsip_inv_session *, pjsip_event *);
	static void        OnMediaUpdate(pjsip_inv_session *, pj_status_t);
	static void        OnTimerStopSession(pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry)
	
private:
	SessionMgr();
	~SessionMgr();

private:
	pj_caching_pool     caching_pool;
	pj_pool_t		   *pool;
	thread              sip_thread;
	pjsip_endpoint	   *sip_endpt;
	pjmedia_endpt      *media_endpt;
	pj_str_t		    local_addr;
	pj_str_t            local_uri;
	pj_uint16_t         sip_port;
	pj_uint16_t         media_port;
	Session             sessions[MAXIMAL_SCREEN_NUM];
	static pj_bool_t    call_report;
	static pj_oshandle_t log_handle;
	static mutex        g_instance_mutex;
	static pj_str_t     POOL_NAME;
	static pj_str_t     LOG_NAME;
	static SessionMgr    *instance;
	static pjsip_module siprtp_module;
	static pjsip_module logger_module;
};

#endif
