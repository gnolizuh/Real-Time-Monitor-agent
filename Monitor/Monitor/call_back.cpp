#include "stdafx.h"
#include "call_back.h"

pj_status_t vid_render_cb(pjmedia_vid_dev_stream *stream, void *user_data, pjmedia_frame *frame)
{
	struct video_stream *video = static_cast<struct video_stream *>(user_data);
	//pj_assert(video != NULL);

	TRACE("vid_render_cb(%p, %p, %p)\n", stream, user_data, frame);

	return PJ_FALSE;
}