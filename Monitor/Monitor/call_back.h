#ifndef __MONITOR_CALL_BACK__
#define __MONITOR_CALL_BACK__

#include "common.h"

pj_status_t rendervid_render_cb_cb(pjmedia_vid_dev_stream *stream, void *user_data, pjmedia_frame *frame);

#endif
