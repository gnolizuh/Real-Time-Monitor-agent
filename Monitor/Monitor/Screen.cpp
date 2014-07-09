#include "stdafx.h"
#include "afxdialogex.h"
#include "Screen.h"

BEGIN_MESSAGE_MAP(Screen, CWnd)
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

void Screen::OnLButtonDown(UINT nFlags, CPoint point)
{
	pj_str_t remote_uri = pj_str("sip:192.168.4.108:5060");
	SessionMgr::GetInstance()->StartSession(&remote_uri, index);
}

Screen::Screen()
	: CWnd()
	, msg_queue()
	, screen_rect(0, 0, 0, 0)
	, wrapper(nullptr)
	, index(0)
	, window(nullptr)
	, render(nullptr)
	, texture(nullptr)
{
}

Screen::~Screen()
{
}

void Screen::Prepare(const CRect &rect, const CWnd *wrapper, pj_uint32_t idx)
{
	pj_uint32_t width = PJ_ABS(rect.right - rect.left);
	pj_uint32_t height = PJ_ABS(rect.bottom - rect.top);

	this->screen_rect = rect;
	this->wrapper = wrapper;
	this->index = idx;

	pj_bool_t ret = this->Create(
		nullptr,
		nullptr,
		WS_BORDER | WS_VISIBLE | WS_CHILD,
		rect,
		(CWnd *)wrapper,
		idx);
	pj_assert(ret == PJ_TRUE);

	window = SDL_CreateWindowFrom(GetSafeHwnd());
	pj_assert(window != nullptr);

	render = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	pj_assert(render != nullptr);

	texture = SDL_CreateTexture(render, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);
	pj_assert(texture != nullptr);

	msg_thread = thread( (std::bind(&Screen::WorkThread, this)) );
}

void Screen::Refresh(const CRect &rect)
{
	lock_guard<mutex> internal_lock(win_mutex);
	this->screen_rect = rect;
	this->MoveWindow(rect);
	this->ShowWindow(SW_SHOW);
}

void Screen::Hide()
{
	lock_guard<mutex> internal_lock(win_mutex);
	SDL_HideWindow(window);
}

void Screen::Painting(const SDL_Rect &rect, const void *pixels, int pitch)
{
	lock_guard<mutex> internal_lock(win_mutex);
	SDL_UpdateTexture( texture, &rect, pixels, pitch );
	SDL_RenderClear( render );
	SDL_RenderCopy( render, texture, nullptr, nullptr );
	SDL_RenderPresent( render );
}

void Screen::WorkThread()
{
	while (1)
	{
		msg_queue.Wait();

		int *package = nullptr;
		do
		{
			package = msg_queue.Pop();
			if ( package )
			{
			}
		} while ( package != nullptr );
	}
}
