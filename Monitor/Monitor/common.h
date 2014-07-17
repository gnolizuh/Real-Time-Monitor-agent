#ifndef __MONITOR_COMMON__
#define __MONITOR_COMMON__

#include <pjlib.h>
#include <pjlib-util.h>
#include <pjnath.h>
#include <pjsip.h>
#include <pjsip_ua.h>
#include <pjsip_simple.h>
#include <pjsua-lib/pjsua.h>
#include <pjmedia.h>
#include <pjmedia-codec.h>

#include <SDL.h>
#include <SDL_thread.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
}

enum { MEDIA_STREAM_AUDIO, MEDIA_STREAM_VIDEO };

struct video_stream
{
	video_stream()
		: tp(NULL)
		, stream(NULL)
		, renderer(NULL)
		, active(PJ_FALSE)
	{
	}

	pjmedia_transport      *tp;
	pjmedia_vid_stream_info info;
	pjmedia_vid_stream     *stream;
	pjmedia_vid_port       *renderer;
	pj_bool_t               active;
};

struct audio_stream
{
	audio_stream()
		: tp(NULL)
		, stream(NULL)
		, active(PJ_FALSE)
	{
	}
    pjmedia_transport   *tp;
	pjmedia_stream_info info;
	pjmedia_stream     *stream;
	pj_bool_t           active;
};

#define MAXIMAL_SCREEN_NUM    9
#define MAXIMAL_THREAD_NUM    1
#define PJ_RETURN_VAL_IF_FALSE(exp, val) do { if ( !(exp) ) return val; } while(0)
#define PJ_RETURN_IF_FALSE(exp) do { if ( !(exp) ) return; } while(0)

#endif
