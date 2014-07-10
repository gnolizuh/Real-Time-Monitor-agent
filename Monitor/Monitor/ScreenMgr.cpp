#include "stdafx.h"
#include "ScreenMgr.h"

ScreenMgr *ScreenMgr::instance = new ScreenMgr();
mutex ScreenMgr::g_instance_mutex;
const resolution_t ScreenMgr::DEFAULT_RESOLUTION = {MININUM_SCREEN_WIDTH * 3 + MININUM_PADDING,
	NIMINUM_SCREEN_HEIGHT * 3 + MININUM_PADDING};

ScreenMgr::ScreenMgr()
	: vertical_padding(MININUM_PADDING)
	, horizontal_padding(MININUM_PADDING)
	, wrapper(nullptr)
	, min_width(MININUM_SCREEN_WIDTH)
	, last_width(MININUM_SCREEN_WIDTH)
	, min_height(NIMINUM_SCREEN_HEIGHT)
	, last_height(NIMINUM_SCREEN_HEIGHT)
	, screen_mgr_res(SCREEN_RES_3x3)
	, screen_mgr_active(PJ_FALSE)
{
	refresh_func.push_back(&ScreenMgr::Refresh_1x1);
	num_blocks.push_back(1);
	refresh_func.push_back(&ScreenMgr::Refresh_2x2);
	num_blocks.push_back(2);
	refresh_func.push_back(&ScreenMgr::Refresh_1x5);
	num_blocks.push_back(3);
	refresh_func.push_back(&ScreenMgr::Refresh_3x3);
	num_blocks.push_back(3);
}

ScreenMgr::~ScreenMgr()
{
}

ScreenMgr *ScreenMgr::GetInstance()
{
	// Thread safety
	lock_guard<mutex> internal_lock(g_instance_mutex);
	pj_assert(instance);
	return instance;
}

resolution_t ScreenMgr::GetDefaultResolution()
{
	return DEFAULT_RESOLUTION;
}

void ScreenMgr::Prepare(CWnd *wrapper)
{
	this->wrapper = wrapper;

	for(pj_uint8_t idx = 0; idx < screens.size(); ++ idx)
	{
		screens[idx]->Prepare(CRect(0, 0, min_width, min_height), (CWnd *)wrapper, idx, IDC_WALL_BASE_INDEX + idx);
	}
}

void ScreenMgr::Prepare(CWnd *wrapper, pj_uint32_t width, pj_uint32_t height, screen_mgr_res_t res)
{
	this->min_width = width;
	this->min_height = height;
	this->screen_mgr_res = res;

	Prepare(wrapper);
}

pj_status_t ScreenMgr::Launch()
{
	(this->* refresh_func[GET_FUNC_INDEX(screen_mgr_res)])();

	screen_mgr_active = PJ_TRUE;

	return PJ_SUCCESS;
}

void ScreenMgr::PushScreenPacket(util_packet_t *packet, pj_uint8_t idx)
{
	screens[idx]->PushPacket(packet);
}

void ScreenMgr::Adjest(pj_int32_t &cx, pj_int32_t &cy)
{
	PJ_RETURN_IF_FALSE(screen_mgr_active);

	// 保存cx, cy
	last_width = cx;
	last_height = cy;
	min_width  = ROUND(cx, num_blocks[GET_FUNC_INDEX(screen_mgr_res)]);
	min_height = ROUND(cy, num_blocks[GET_FUNC_INDEX(screen_mgr_res)]);

	(this->* refresh_func[GET_FUNC_INDEX(screen_mgr_res)])();
}

void ScreenMgr::Refresh(screen_mgr_res_t res)
{
	PJ_RETURN_IF_FALSE(this->screen_mgr_res != res);

	this->screen_mgr_res = res;

	// 沿用上一次的cx,cy. 以免窗口大小被计算后越来越小.
	pj_uint32_t tmp_width = last_width, tmp_height = last_height;
	min_width  = ROUND(tmp_width, num_blocks[GET_FUNC_INDEX(screen_mgr_res)]);
	min_height = ROUND(tmp_height, num_blocks[GET_FUNC_INDEX(screen_mgr_res)]);
	
	HideAll();

	(this->* refresh_func[GET_FUNC_INDEX(screen_mgr_res)])();
}

void ScreenMgr::Refresh_1x1()
{
	screens[0]->Refresh(CRect(0, 0, min_width, min_height));
}

void ScreenMgr::Refresh_2x2()
{
	const unsigned MAX_COL = 2, MAX_ROW = 2;
	for ( unsigned col = 0; col < MAX_COL; ++ col )
	{
		for ( unsigned row = 0; row < MAX_ROW; ++ row )
		{
			unsigned idx = col * MAX_COL + row;
			CRect rect;
			rect.left  = col * min_width + col * horizontal_padding;
			rect.top   = row * min_height + row * vertical_padding;
			rect.right = rect.left + min_width;
			rect.bottom = rect.top + min_height;

			screens[idx]->Refresh(rect);
		}
	}
}

void ScreenMgr::Refresh_1x5()
{
	unsigned idx = 0;

	pj_int32_t left = 0, top = 0, right, bottom;
	right = min_width * 2 + horizontal_padding;
	bottom = min_height * 2 + vertical_padding;
	screens[idx ++]->Refresh(CRect(left, top, right, bottom));

	left = right + horizontal_padding;
	right = left + min_width;
	bottom = min_height;
	screens[idx ++]->Refresh(CRect(left, top, right, bottom));

	top = bottom + vertical_padding;
	bottom = top + min_height;
	screens[idx ++]->Refresh(CRect(left, top, right, bottom));

	left = 0;
	right = min_width;
	top = bottom + vertical_padding;
	bottom = top + min_height;
	screens[idx ++]->Refresh(CRect(left, top, right, bottom));

	left = right + horizontal_padding;
	right = left + min_width;
	screens[idx ++]->Refresh(CRect(left, top, right, bottom));

	left = right + horizontal_padding;
	right = left + min_width;
	screens[idx ++]->Refresh(CRect(left, top, right, bottom));
}

void ScreenMgr::Refresh_3x3()
{
	const unsigned MAX_COL = 3, MAX_ROW = 3;
	for ( unsigned col = 0; col < MAX_COL; ++ col )
	{
		for ( unsigned row = 0; row < MAX_ROW; ++ row )
		{
			unsigned idx = col * MAX_COL + row;
			CRect rect;
			rect.left  = col * min_width + col * horizontal_padding;
			rect.top   = row * min_height + row * vertical_padding;
			rect.right = rect.left + min_width;
			rect.bottom = rect.top + min_height;

			screens[idx]->Refresh(rect);
		}
	}
}

void ScreenMgr::HideAll()
{
	for(pj_uint8_t idx = 0; idx < screens.size(); ++ idx)
	{
		screens[idx]->Hide();
	}
}
