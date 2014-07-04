#include "stdafx.h"
#include "afxdialogex.h"
#include "Screen.h"

BEGIN_MESSAGE_MAP(Screen, CWnd)
END_MESSAGE_MAP()

const CWnd *Screen::g_wrapper = NULL;

void Screen::SetWrapper(const CWnd *wrapper)
{
	g_wrapper = wrapper;
}

Screen::Screen()
	: CWnd()
{
}

Screen::~Screen()
{
}

void Screen::Prepare(pj_uint32_t width, pj_uint32_t height)
{

}

pj_status_t Screen::Launch()
{
	return PJ_SUCCESS;
}

