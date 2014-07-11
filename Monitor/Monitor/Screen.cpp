#include "stdafx.h"
#include "afxdialogex.h"
#include "Screen.h"

BEGIN_MESSAGE_MAP(Screen, CWnd)
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

void Screen::OnLButtonDown(UINT nFlags, CPoint point)
{
	pj_str_t remote_uri = pj_str("sip:192.168.4.108:5060");
	if ( ( call_status ++ ) % 2 == 0 )
	{
		SessionMgr::GetInstance()->StartSession(&remote_uri, idx);
	}
	else
	{
		SessionMgr::GetInstance()->StopSession(idx);
	}
}

Screen::Screen()
	: CWnd()
	, msg_queue()
	, screen_rect(0, 0, 0, 0)
	, wrapper(nullptr)
	, idx(0)
	, id(0)
	, window(nullptr)
	, render(nullptr)
	, texture(nullptr)
	, call_status(0)
{
}

Screen::~Screen()
{
}

void Screen::Prepare(const CRect &rect, const CWnd *wrapper, pj_uint32_t idx, pj_uint32_t id)
{
	pj_uint32_t width = PJ_ABS(rect.right - rect.left);
	pj_uint32_t height = PJ_ABS(rect.bottom - rect.top);

	this->screen_rect = rect;
	this->wrapper = wrapper;
	this->idx = idx;
	this->id  = id;

	pj_bool_t ret = this->Create(
		nullptr,
		nullptr,
		WS_BORDER | WS_VISIBLE | WS_CHILD,
		rect,
		(CWnd *)wrapper,
		id);
	pj_assert(ret == PJ_TRUE);

	window = SDL_CreateWindowFrom(GetSafeHwnd());
	pj_assert(window != nullptr);

	render = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	pj_assert(render != nullptr);

	texture = SDL_CreateTexture(render, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);
	pj_assert(texture != nullptr);

	msg_thread = thread( (std::bind(&Screen::MessageQueueThread, this)) );
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

void Screen::PushPacket(util_packet_t *packet)
{
	msg_queue.Push(packet);
}

void Screen::MessageQueueThread()
{
	while (1)
	{
		msg_queue.Wait();

		util_packet_t *packet = nullptr;
		do
		{
			packet = msg_queue.Pop();
			if ( packet )
			{
				ProcessMessage(packet);

				if (packet->buf)
				{
					delete packet->buf;
					packet->buf = NULL;
				}

				delete packet;
				packet = NULL;
			}
		} while ( packet != nullptr );
	}
}

void Screen::ProcessMessage(util_packet_t *packet)
{
	PJ_RETURN_IF_FALSE(packet != NULL);

	switch(packet->type)
	{
		case sinashow::UTIL_PACKET_SIP:
		{
			ProcessSipMessage(packet);
			break;
		}
		case sinashow::UTIL_PACKET_MEDIA:
		{
			ProcessMediaMessage(packet);
			break;
		}
	}
}

void Screen::ProcessSipMessage(util_packet_t *packet)
{
	util_sip_packet_type_t code = *(util_sip_packet_type_t *)packet->buf;

	//PJ_LOG(5, (THIS_FILE, "code = %u", code));

	return;
}

void Screen::ProcessMediaMessage(util_packet_t *packet)
{
	util_sip_packet_type_t code = *(util_sip_packet_type_t *)packet->buf;

	//PJ_LOG(5, (THIS_FILE, "code = %u", code));

	return;
}
