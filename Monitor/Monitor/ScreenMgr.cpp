#include "stdafx.h"
#include "ScreenMgr.h"

ScreenMgr *ScreenMgr::instance = NULL;

ScreenMgr::ScreenMgr()
	: PADDING(1)
	, wrapper(NULL)
	, min_width(0)
	, min_height(0)
{
}

ScreenMgr::~ScreenMgr()
{
}

// Static function
ScreenMgr *ScreenMgr::GetInstance()
{
	if (!instance)
	{
		instance = new ScreenMgr();
		pj_assert(instance);
	}
	return instance;
}

void ScreenMgr::Prepare(CWnd *wrapper, pj_uint32_t width, pj_uint32_t height, screen_mgr_res_t res)
{
	this->wrapper = wrapper;
	this->min_width = width;
	this->min_height = height;
	this->res = res;
}

pj_status_t ScreenMgr::Launch()
{
	const unsigned MAX_COL = 3, MAX_ROW = 3;
	for ( unsigned col = 0; col < MAX_COL; ++ col )
	{
		for ( unsigned row = 0; row < MAX_ROW; ++ row )
		{
			unsigned idx = col * MAX_COL + row;
			CRect rect;
			rect.left  = col * min_width + col * PADDING;
			rect.top   = row * min_height + row * PADDING;
			rect.right = rect.left + min_width;
			rect.bottom = rect.top + min_height;

			wall[idx].Create(
				NULL,
				NULL,
				WS_BORDER | WS_VISIBLE | WS_CHILD,
				rect,
				(CWnd *)wrapper,
				IDC_WALL_BASE_INDEX + idx);
		}
	}

	switch(res)
	{
		case SCREEN_RES_ONE:
		{
			Flex_ONE();
			break;
		}
		case SCREEN_RES_2x2:
		{
			Flex_2x2();
			break;
		}
		case SCREEN_RES_1x5:
		{
			Flex_1x5();
			break;
		}
		case SCREEN_RES_3x3:
		{
			Flex_3x3();
			break;
		}
	}

	return PJ_SUCCESS;
}

void ScreenMgr::Flex(pj_uint32_t width, pj_uint32_t height, screen_mgr_res_t res)
{
	if ( this->res == res &&
		this->min_width == width &&
		this->min_height == height )
	{
		return;
	}

	this->res = res;
	this->min_width = width;
	this->min_height = height;
	switch(res)
	{
		case SCREEN_RES_ONE:
		{
			Flex_ONE();
			break;
		}
		case SCREEN_RES_2x2:
		{
			Flex_2x2();
			break;
		}
		case SCREEN_RES_1x5:
		{
			Flex_1x5();
			break;
		}
		case SCREEN_RES_3x3:
		{
			Flex_3x3();
			break;
		}
	}
}

void ScreenMgr::Flex_ONE()
{
	Flex_hide();

	wall[0].MoveWindow(CRect(0, 0, min_width, min_height));
	wall[0].ShowWindow(SW_SHOW);
}

void ScreenMgr::Flex_2x2()
{
	Flex_hide();

	const unsigned MAX_COL = 2, MAX_ROW = 2;
	for ( unsigned col = 0; col < MAX_COL; ++ col )
	{
		for ( unsigned row = 0; row < MAX_ROW; ++ row )
		{
			unsigned idx = col * MAX_COL + row;
			CRect rect;
			rect.left  = col * min_width + col * PADDING;
			rect.top   = row * min_height + row * PADDING;
			rect.right = rect.left + min_width;
			rect.bottom = rect.top + min_height;

			wall[idx].MoveWindow(rect);
			wall[idx].ShowWindow(SW_SHOW);
		}
	}
}

void ScreenMgr::Flex_1x5()
{
	Flex_hide();
	unsigned idx = 0;

	pj_int32_t left = 0, top = 0, right, bottom;
	right = min_width * 2 + PADDING;
	bottom = min_height * 2 + PADDING;
	wall[idx].MoveWindow(CRect(left, top, right, bottom)); // Index 0, the bigest one.
	wall[idx ++].ShowWindow(SW_SHOW);

	left = right + PADDING;
	right = left + min_width;
	bottom = min_height;
	wall[idx].MoveWindow(CRect(left, top, right, bottom)); // Index 1
	wall[idx ++].ShowWindow(SW_SHOW);

	top = bottom + PADDING;
	bottom = top + min_height;
	wall[idx].MoveWindow(CRect(left, top, right, bottom));
	wall[idx ++].ShowWindow(SW_SHOW);

	left = 0;
	right = min_width;
	top = bottom + PADDING;
	bottom = top + min_height;
	wall[idx].MoveWindow(CRect(left, top, right, bottom));
	wall[idx ++].ShowWindow(SW_SHOW);

	left = right + PADDING;
	right = left + min_width;
	wall[idx].MoveWindow(CRect(left, top, right, bottom));
	wall[idx ++].ShowWindow(SW_SHOW);

	left = right + PADDING;
	right = left + min_width;
	wall[idx].MoveWindow(CRect(left, top, right, bottom));
	wall[idx ++].ShowWindow(SW_SHOW);
}

void ScreenMgr::Flex_3x3()
{
	Flex_hide();

	const unsigned MAX_COL = 3, MAX_ROW = 3;
	for ( unsigned col = 0; col < MAX_COL; ++ col )
	{
		for ( unsigned row = 0; row < MAX_ROW; ++ row )
		{
			unsigned idx = col * MAX_COL + row;
			CRect rect;
			rect.left  = col * min_width + col * PADDING;
			rect.top   = row * min_height + row * PADDING;
			rect.right = rect.left + min_width;
			rect.bottom = rect.top + min_height;

			wall[idx].MoveWindow(rect);
			wall[idx].ShowWindow(SW_SHOW);
		}
	}
}

void ScreenMgr::Flex_hide()
{
	for ( unsigned idx = 0; idx < PJ_ARRAY_SIZE(wall); ++ idx )
	{
		wall[idx].ShowWindow(SW_HIDE);
	}
}