#ifndef __MONITOR_SCREEN_MGR__
#define __MONITOR_SCREEN_MGR__

#include "Resource.h"
#include "common.h"
#include "Screen.h"

typedef enum 
{
	SCREEN_RES_ONE,
	SCREEN_RES_2x2,
	SCREEN_RES_1x5,
	SCREEN_RES_3x3,
} screen_mgr_res_t;

class ScreenMgr
{
public:
	// Singleton
	static ScreenMgr *GetInstance();
	void Prepare(CWnd *, pj_uint32_t, pj_uint32_t, screen_mgr_res_t);
	pj_status_t Launch();
	void Flex(pj_uint32_t, pj_uint32_t, screen_mgr_res_t);

private:
	ScreenMgr();
	virtual ~ScreenMgr();
	void Flex_ONE();
	void Flex_2x2();
	void Flex_1x5();
	void Flex_3x3();
	void Flex_hide();

private:
	Screen wall[9];
	const CWnd *wrapper;
	pj_uint32_t min_width, min_height;
	screen_mgr_res_t res;

	static ScreenMgr *instance;
	const pj_uint8_t PADDING;
};

#endif
