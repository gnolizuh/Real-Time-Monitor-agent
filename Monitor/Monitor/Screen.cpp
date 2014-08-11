#include "stdafx.h"
#include "afxdialogex.h"
#include "Screen.h"

BEGIN_MESSAGE_MAP(Screen, CWnd)
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

Screen::Screen(pj_uint32_t index)
	: CWnd()
	, rect_(0, 0, 0, 0)
	, wrapper_(NULL)
	, index_(index)
	, uid_(0)
	, window_(NULL)
	, render_(NULL)
	, texture_(NULL)
	, mutex_()
	, sync_thread_pool_(1)
	, call_status(0)
{
}

Screen::~Screen()
{
}

pj_status_t Screen::Prepare(const CRect &rect, const CWnd *wrapper, pj_uint32_t uid)
{
	pj_uint32_t width = PJ_ABS(rect.right - rect.left);
	pj_uint32_t height = PJ_ABS(rect.bottom - rect.top);

	set_rect(rect);
	set_wrapper(wrapper);
	set_uid(uid);

	pj_bool_t ret = this->Create(
		nullptr,
		nullptr,
		WS_BORDER | WS_VISIBLE | WS_CHILD,
		rect,
		(CWnd *)wrapper,
		this->uid());
	pj_assert(ret == PJ_TRUE);

	set_window(SDL_CreateWindowFrom(GetSafeHwnd()));
	pj_assert(window() != NULL);

	set_render(SDL_CreateRenderer(window(), -1, SDL_RENDERER_SOFTWARE));
	pj_assert(render() != NULL);

	set_texture(SDL_CreateTexture(render(), SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height));
	pj_assert(texture() != NULL);

	sync_thread_pool_.Start();

	return PJ_SUCCESS;
}

pj_status_t Screen::Launch()
{
	return PJ_SUCCESS;
}

void Screen::Refresh(const CRect &rect)
{
	lock_guard<std::mutex> internal_lock(mutex_);
	set_rect(rect);

	this->MoveWindow(rect);
	this->ShowWindow(SW_SHOW);
}

void Screen::Hide()
{
	lock_guard<std::mutex> internal_lock(mutex_);
	SDL_HideWindow(window());
}

void Screen::Painting(const SDL_Rect &rect, const void *pixels, int pitch)
{
	lock_guard<std::mutex> internal_lock(mutex_);
	SDL_UpdateTexture(texture(), &rect, pixels, pitch);
	SDL_RenderClear(render());
	SDL_RenderCopy(render(), texture(), NULL, NULL);
	SDL_RenderPresent(render());
}

void Screen::OnLButtonDown(UINT nFlags, CPoint point)
{
	pj_str_t remote_uri = pj_str("sip:192.168.6.40:6080");/*"sip:192.168.4.108:5060"*/
	if ( ( call_status ++ ) % 2 == 0 )
	{
		/*SessionMgr::GetInstance()->StartSession(&remote_uri, index());*/
	}
	else
	{
		/*SessionMgr::GetInstance()->StopSession(index());*/
	}
}
