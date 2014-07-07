#ifndef __MONITOR_SCREEN__
#define __MONITOR_SCREEN__

#include "common.h"

class Screen
	: public CWnd
{
public:
	Screen();
	virtual ~Screen();
	void Prepare(const CRect &, const CWnd *, pj_uint32_t);
	void Refresh(const CRect &);

protected:
	DECLARE_MESSAGE_MAP()

private:
	CRect screen_rect;
	const CWnd *wrapper;
	pj_uint32_t index;
	SDL_Window *window;
};

#endif
