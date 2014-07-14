#ifndef __MONITOR_SCREEN_MGR__
#define __MONITOR_SCREEN_MGR__

#include <vector>
#include <thread>
#include <mutex>
#include "MessageQueue.hpp"
#include "UtilPacket.h"
#include "Resource.h"
#include "common.h"
#include "SessionMgr.h"
#include "Screen.h"

#define MININUM_PADDING       0
#define MININUM_SCREEN_WIDTH  176
#define NIMINUM_SCREEN_HEIGHT 144
#define ROUND(num, fraction) (num) /= (fraction), (num) *= (fraction), (num) / (fraction)
#define GET_FUNC_INDEX(idx) ((idx) < 0 || (idx) >= SCREEN_RES_END ? SCREEN_RES_END : (idx))

typedef enum
{
	SCREEN_RES_1x1,
	SCREEN_RES_2x2,
	SCREEN_RES_1x5,
	SCREEN_RES_3x3,
	SCREEN_RES_END,            // useless
} screen_mgr_res_t;

typedef struct
{
	pj_uint32_t x;
	pj_uint32_t y;
} resolution_t;

using std::mutex;
using std::lock_guard;
using std::vector;
using sinashow::MessageQueue;
using sinashow::util_packet_t;

class ScreenMgr;
typedef void (ScreenMgr::*screenmgr_func_t)(pj_uint32_t, pj_uint32_t);

class Screen;

class ScreenMgr
{
public:
	static ScreenMgr *GetInstance();                                         // Singleton
	void Prepare(CWnd *);
	void Prepare(CWnd *, pj_uint32_t, pj_uint32_t, screen_mgr_res_t);
	void Refresh(screen_mgr_res_t);
	void Adjest(pj_int32_t &, pj_int32_t &);
	void HideAll();
	pj_status_t Launch();
	void PushScreenPacket(util_packet_t *, pj_uint8_t);
	static resolution_t GetDefaultResolution();
	MessageQueue<util_packet_t *> *GetMessageQueue(pj_uint8_t);

private:
	ScreenMgr();                                                             // Construct by user is forbitten.
	virtual ~ScreenMgr();

	void Refresh_1x1(pj_uint32_t, pj_uint32_t);
	void Refresh_2x2(pj_uint32_t, pj_uint32_t);
	void Refresh_1x5(pj_uint32_t, pj_uint32_t);
	void Refresh_3x3(pj_uint32_t, pj_uint32_t);

private:
	inline const CWnd *wrapper() { return wrapper_; }
	inline vector<Screen *> &screens() { return screens_; }
	inline pj_uint32_t width() const { return width_; }
	inline pj_uint32_t height() const { return height_; }
	inline screen_mgr_res_t screen_mgr_res() const { return screen_mgr_res_; }
	inline pj_uint32_t vertical_padding() const { return vertical_padding_; }
	inline pj_uint32_t horizontal_padding() const { return horizontal_padding_; }
	inline pj_bool_t screen_mgr_active() const { return screen_mgr_active_; }
	inline vector<screenmgr_func_t> &screenmgr_func_array() { return screenmgr_func_array_; }
	inline vector<pj_uint32_t> &num_blocks() { return num_blocks_; }

	inline void set_wrapper(const CWnd *wrapper) { wrapper_ = wrapper; }
	inline void set_width(pj_uint32_t width) { width_ = width; }
	inline void set_height(pj_uint32_t height) { height_ = height; }
	inline void set_screen_mgr_res(screen_mgr_res_t res) { screen_mgr_res_ = res; }
	inline void set_vertical_padding(pj_uint32_t padding) { vertical_padding_ = padding; }
	inline void set_horizontal_padding(pj_uint32_t padding) { horizontal_padding_ = padding; }
	inline void set_screen_mgr_active(pj_bool_t active) { screen_mgr_active_ = active; }

private:
	const CWnd      *wrapper_;
	vector<Screen *> screens_;
	pj_uint32_t      width_;
	pj_uint32_t      height_;
	screen_mgr_res_t screen_mgr_res_;
	pj_uint32_t      vertical_padding_;
	pj_uint32_t      horizontal_padding_;
	pj_bool_t        screen_mgr_active_;
	vector<screenmgr_func_t> screenmgr_func_array_;
	vector<pj_uint32_t> num_blocks_;

	static ScreenMgr *g_instance_;
	static mutex g_instance_mutex_;
	static const resolution_t DEFAULT_RESOLUTION;
};

#endif
