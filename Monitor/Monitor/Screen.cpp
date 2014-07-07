#include "stdafx.h"
#include "afxdialogex.h"
#include "Screen.h"

BEGIN_MESSAGE_MAP(Screen, CWnd)
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
}

void Screen::Refresh(const CRect &rect)
{
	this->screen_rect = rect;

	this->MoveWindow(rect);
	this->UpdateWindow();
}
