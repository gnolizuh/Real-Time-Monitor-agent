#include "stdafx.h"
#include "afxdialogex.h"
#include "Screen.h"

BEGIN_MESSAGE_MAP(Screen, CWnd)
	ON_WM_PAINT()
END_MESSAGE_MAP()

Screen::Screen()
	: CWnd()
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
		NULL,
		NULL,
		WS_BORDER | WS_VISIBLE | WS_CHILD,
		rect,
		(CWnd *)wrapper,
		idx);
	pj_assert(ret == PJ_TRUE);

	window = SDL_CreateWindowFrom(GetSafeHwnd());
	pj_assert(window != nullptr);

	render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	pj_assert(render != nullptr);

	texture = SDL_CreateTexture(render, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);
	pj_assert(texture != nullptr);
}

void Screen::Refresh(const CRect &rect)
{
	pj_uint32_t width = PJ_ABS(rect.right - rect.left);
	pj_uint32_t height = PJ_ABS(rect.bottom - rect.top);
	this->screen_rect = rect;

	if (texture)
	{
		SDL_DestroyTexture(texture);
		texture = SDL_CreateTexture(render, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);
		pj_assert(render != nullptr);
	}

	this->MoveWindow(rect);
	this->ShowWindow(SW_SHOW);
}

void Screen::Hide()
{
	this->ShowWindow(SW_HIDE);
}

void Screen::OnPaint()
{
	CWnd::OnPaint();
}

void Screen::Painting()
{
	int const WIDTH = PJ_ABS(screen_rect.right - screen_rect.left);
	int const HEIGHT = PJ_ABS(screen_rect.bottom - screen_rect.top);
	int left_pitch = WIDTH * SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_IYUV);
	SDL_Rect left_rect = {0, 0, WIDTH, HEIGHT};

	int buf_size = WIDTH * HEIGHT * 3 / 2;
	uint8_t *buf = (uint8_t *)malloc(buf_size);

	memset(buf, 1, buf_size);

	int ret;
	ret = SDL_UpdateTexture(texture, &left_rect, buf, left_pitch);
	ret = SDL_RenderClear(render);
	ret = SDL_RenderCopy(render, texture, NULL, NULL);
	SDL_RenderPresent(render);

	UpdateWindow();
}