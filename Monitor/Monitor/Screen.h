#ifndef __MONITOR_SCREEN__
#define __MONITOR_SCREEN__

#include <thread>
#include <mutex>
#include "common.h"
#include "SessionMgr.h"
#include "MessageQueue.hpp"

using std::lock_guard;
using std::thread;
using std::mutex;

class Screen
	: public CWnd
{
public:
	Screen();
	virtual ~Screen();
	void Prepare(const CRect &, const CWnd *, pj_uint32_t);
	void Refresh(const CRect &);
	void Hide();
	void Painting(const SDL_Rect &, const void *, int);

protected:
	void WorkThread();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()

private:
	CRect               screen_rect;
	const CWnd         *wrapper;
	pj_uint32_t         index;
	SDL_Window         *window;       // SDL´°¿Ú
	SDL_Renderer       *render;       // äÖÈ¾Æ÷
	SDL_Texture        *texture;      // ÎÆÀí
	mutex               win_mutex;
	sinashow::MessageQueue<int *> msg_queue;
	thread              msg_thread;
};

#endif
