#ifndef __MONITOR_SCREEN__
#define __MONITOR_SCREEN__

#include <thread>
#include <mutex>
#include "MessageQueue.hpp"
#include "ScreenMgr.h"
#include "UtilPacket.h"
#include "common.h"

using std::lock_guard;
using std::thread;
using std::mutex;
using sinashow::MessageQueue;
using sinashow::util_packet_t;
using sinashow::util_sip_packet_type_t;

class Screen
	: public CWnd
{
public:
	Screen(pj_uint32_t);
	virtual ~Screen();
	void Prepare(const CRect &, const CWnd *, pj_uint32_t);
	void Refresh(const CRect &);
	void Hide();
	void Painting(const SDL_Rect &, const void *, int);
	void PushPacket(util_packet_t *);
	MessageQueue<util_packet_t *> *GetMessageQueue();

protected:
	void MessageQueueThread();
	void ProcessMessage(util_packet_t *);
	void ProcessSipMessage(util_packet_t *);
	void ProcessMediaMessage(util_packet_t *);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()

private:
	pj_uint32_t         call_status;
	CRect               screen_rect;
	const CWnd         *wrapper;
	const pj_uint32_t   index;
	pj_uint32_t         id;
	SDL_Window         *window;       // SDL´°¿Ú
	SDL_Renderer       *render;       // SDLäÖÈ¾Æ÷
	SDL_Texture        *texture;      // SDLÎÆÀí
	mutex               win_mutex;
	thread              msg_thread;
	MessageQueue<util_packet_t *> msg_queue;
};

#endif
