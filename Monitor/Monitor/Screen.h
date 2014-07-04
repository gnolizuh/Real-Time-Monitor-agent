#ifndef __MONITOR_SCREEN__
#define __MONITOR_SCREEN__

#include "common.h"

class Screen
	: public CWnd
{
public:
	Screen();
	virtual ~Screen();
	void Prepare(pj_uint32_t, pj_uint32_t);
	pj_status_t Launch();
	static void SetWrapper(const CWnd *);

protected:
	DECLARE_MESSAGE_MAP()

private:
	pj_uint32_t width, height;
	SDL_Window *window;

	static const CWnd *g_wrapper;
};

#endif
