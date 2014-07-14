#include "stdafx.h"
#include "ScreenMgr.h"

ScreenMgr *ScreenMgr::g_instance_ = new ScreenMgr();
mutex ScreenMgr::g_instance_mutex_;
const resolution_t ScreenMgr::DEFAULT_RESOLUTION = {MININUM_SCREEN_WIDTH * 3 + MININUM_PADDING,
	NIMINUM_SCREEN_HEIGHT * 3 + MININUM_PADDING};

ScreenMgr::ScreenMgr()
	: wrapper_(NULL)
	, screens_(MAXIMAL_SCREEN_NUM)
	, width_(MININUM_SCREEN_WIDTH)
	, height_(NIMINUM_SCREEN_HEIGHT)
	, screen_mgr_res_(SCREEN_RES_3x3)
	, vertical_padding_(MININUM_PADDING)
	, horizontal_padding_(MININUM_PADDING)
	, screen_mgr_active_(PJ_FALSE)
	, screenmgr_func_array_()
	, num_blocks_()
{
	screenmgr_func_array_.push_back(&ScreenMgr::Refresh_1x1);
	num_blocks_.push_back(1);
	screenmgr_func_array_.push_back(&ScreenMgr::Refresh_2x2);
	num_blocks_.push_back(2);
	screenmgr_func_array_.push_back(&ScreenMgr::Refresh_1x5);
	num_blocks_.push_back(3);
	screenmgr_func_array_.push_back(&ScreenMgr::Refresh_3x3);
	num_blocks_.push_back(3);

	for (pj_uint32_t idx = 0; idx < MAXIMAL_SCREEN_NUM; ++ idx)
	{
		screens()[idx] = new Screen(idx);
	}
}

ScreenMgr::~ScreenMgr()
{
}

ScreenMgr *ScreenMgr::GetInstance()
{
	// Thread safety
	lock_guard<mutex> internal_lock(g_instance_mutex_);
	pj_assert(g_instance_);
	return g_instance_;
}

resolution_t ScreenMgr::GetDefaultResolution()
{
	return DEFAULT_RESOLUTION;
}

MessageQueue<util_packet_t *> *ScreenMgr::GetMessageQueue(pj_uint8_t index)
{
	return screens()[index]->GetMessageQueue();
}

void ScreenMgr::Prepare(CWnd *wrapper)
{
	set_wrapper(wrapper);

	for(pj_uint32_t idx = 0; idx < screens().size(); ++ idx)
	{
		screens()[idx]->Prepare(CRect(0, 0, width(), height()), (CWnd *)wrapper, IDC_WALL_BASE_INDEX + idx);
	}
}

void ScreenMgr::Prepare(CWnd *wrapper, pj_uint32_t width, pj_uint32_t height, screen_mgr_res_t res)
{
	set_width(width);
	set_height(height);
	set_screen_mgr_res(res);

	Prepare(wrapper);
}

pj_status_t ScreenMgr::Launch()
{
	(this->* screenmgr_func_array()[GET_FUNC_INDEX(screen_mgr_res())])(width(), height());

	set_screen_mgr_active(PJ_TRUE);

	return PJ_SUCCESS;
}

void ScreenMgr::PushScreenPacket(util_packet_t *packet, pj_uint8_t idx)
{
	screens()[idx]->PushPacket(packet);
}

void ScreenMgr::Adjest(pj_int32_t &cx, pj_int32_t &cy)
{
	PJ_RETURN_IF_FALSE(screen_mgr_active());

	set_width(cx);
	set_height(cy);

	pj_uint32_t divisor = num_blocks()[GET_FUNC_INDEX(screen_mgr_res())];
	pj_uint32_t round_width, round_height;
	round_width  = ROUND(cx, divisor);
	round_height = ROUND(cy, divisor);

	(this->* screenmgr_func_array()[GET_FUNC_INDEX(screen_mgr_res())])(round_width, round_height);
}

void ScreenMgr::Refresh(screen_mgr_res_t res)
{
	PJ_RETURN_IF_FALSE(screen_mgr_res() != res);

	set_screen_mgr_res(res);

	// 沿用上一次的cx,cy. 以免窗口大小被计算后越来越小.
	pj_uint32_t divisor = num_blocks()[GET_FUNC_INDEX(screen_mgr_res())];
	pj_uint32_t width = this->width(), height = this->height();
	pj_uint32_t round_width, round_height;
	round_width = ROUND(width, divisor);
	round_height = ROUND(height, divisor);
	
	HideAll();

	(this->* screenmgr_func_array()[GET_FUNC_INDEX(screen_mgr_res())])(round_width, round_height);
}

void ScreenMgr::Refresh_1x1(pj_uint32_t width, pj_uint32_t height)
{
	screens()[0]->Refresh(CRect(0, 0, width, height));
}

void ScreenMgr::Refresh_2x2(pj_uint32_t width, pj_uint32_t height)
{
	const unsigned MAX_COL = 2, MAX_ROW = 2;
	for ( unsigned col = 0; col < MAX_COL; ++ col )
	{
		for ( unsigned row = 0; row < MAX_ROW; ++ row )
		{
			unsigned idx = col * MAX_COL + row;
			CRect rect;
			rect.left  = col * width + col * horizontal_padding();
			rect.top   = row * height + row * vertical_padding();
			rect.right = rect.left + width;
			rect.bottom = rect.top + height;

			screens()[idx]->Refresh(rect);
		}
	}
}

void ScreenMgr::Refresh_1x5(pj_uint32_t width, pj_uint32_t height)
{
	unsigned idx = 0;

	pj_int32_t left = 0, top = 0, right, bottom;
	right = width * 2 + horizontal_padding();
	bottom = height * 2 + vertical_padding();
	screens()[idx ++]->Refresh(CRect(left, top, right, bottom));

	left = right + horizontal_padding();
	right = left + width;
	bottom = height;
	screens()[idx ++]->Refresh(CRect(left, top, right, bottom));

	top = bottom + vertical_padding();
	bottom = top + height;
	screens()[idx ++]->Refresh(CRect(left, top, right, bottom));

	left = 0;
	right = width;
	top = bottom + vertical_padding();
	bottom = top + height;
	screens()[idx ++]->Refresh(CRect(left, top, right, bottom));

	left = right + horizontal_padding();
	right = left + width;
	screens()[idx ++]->Refresh(CRect(left, top, right, bottom));

	left = right + horizontal_padding();
	right = left + width;
	screens()[idx ++]->Refresh(CRect(left, top, right, bottom));
}

void ScreenMgr::Refresh_3x3(pj_uint32_t width, pj_uint32_t height)
{
	const unsigned MAX_COL = 3, MAX_ROW = 3;
	for ( unsigned col = 0; col < MAX_COL; ++ col )
	{
		for ( unsigned row = 0; row < MAX_ROW; ++ row )
		{
			unsigned idx = col * MAX_COL + row;
			CRect rect;
			rect.left  = col * width + col * horizontal_padding();
			rect.top   = row * height + row * vertical_padding();
			rect.right = rect.left + width;
			rect.bottom = rect.top + height;

			screens()[idx]->Refresh(rect);
		}
	}
}

void ScreenMgr::HideAll()
{
	for(pj_uint8_t idx = 0; idx < screens().size(); ++ idx)
	{
		screens()[idx]->Hide();
	}
}
