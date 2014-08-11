#ifndef __MONITOR_SCREEN__
#define __MONITOR_SCREEN__

#include <thread>
#include <mutex>
#include "PoolThread.hpp"
#include "ScreenMgr.h"

using std::lock_guard;
using std::mutex;
using std::thread;

class Screen
	: public CWnd
{
public:
	Screen(pj_uint32_t);
	virtual ~Screen();
	pj_status_t Prepare(const CRect &, const CWnd *, pj_uint32_t);
	pj_status_t Launch();
	void Refresh(const CRect &);
	void Hide();
	void Painting(const SDL_Rect &, const void *, int);

protected:
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
	PoolThread<std::function<void ()>> sync_thread_pool_;

	pj_uint32_t       call_status;
};

#endif
