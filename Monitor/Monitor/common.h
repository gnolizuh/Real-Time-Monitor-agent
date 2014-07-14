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

#define MAXIMAL_SCREEN_NUM    9
#define PJ_RETURN_VAL_IF_FALSE(exp, val) do { if ( !(exp) ) return val; } while(0)
#define PJ_RETURN_IF_FALSE(exp) do { if ( !(exp) ) return; } while(0)
#define GENERATE_ACCESSOR(_macro_name_, _macro_type_) \
	_macro_type_ _macro_name_() const { return _macro_name_##_; } \
	void set_##_macro_name_(_macro_type_ arg) { _macro_name_##_ = arg; }

#endif
