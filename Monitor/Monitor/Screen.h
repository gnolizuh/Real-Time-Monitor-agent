#ifndef __MONITOR_SCREEN__
#define __MONITOR_SCREEN__

#include <thread>
#include <mutex>
#include "MessageQueue.hpp"
#include "ScreenMgr.h"
#include "UtilPacket.h"
#include "common.h"

using std::lock_guard;
using std::mutex;
using std::thread;
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
	inline const CWnd *wrapper() const { return wrapper_; }
	inline const pj_uint32_t index() const { return index_; }
	inline CRect rect() const { return rect_; }
	inline pj_uint32_t uid() const { return uid_; }
	inline SDL_Window *window() const { return window_; }
	inline SDL_Renderer *render() const { return render_; }
	inline SDL_Texture *texture() const { return texture_; }
	inline MessageQueue<util_packet_t *> &msg_queue() { return msg_queue_; }

	inline void set_wrapper(const CWnd *wrapper) { wrapper_ = wrapper; }
	inline void set_rect(CRect rect) { rect_ = rect; }
	inline void set_uid(pj_uint32_t uid) { uid_ = uid; }
	inline void set_window(SDL_Window *window) { window_ = window; }
	inline void set_render(SDL_Renderer *render) { render_ = render; }
	inline void set_texture(SDL_Texture *texture) { texture_ = texture; }

private:
	const CWnd       *wrapper_;
	const pj_uint32_t index_;
	std::mutex        mutex_;
	CRect             rect_;
	pj_uint32_t       uid_;
	SDL_Window       *window_;
	SDL_Renderer     *render_;
	SDL_Texture      *texture_;
	std::thread       thread_;
	MessageQueue<util_packet_t *> msg_queue_;

	pj_uint32_t       call_status;
};

#endif
