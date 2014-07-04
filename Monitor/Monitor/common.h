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

#define GEN_GET_SET(_macro_name_, _macro_type_) \
	_macro_type_ _macro_name_() const { return _macro_name_##_; } \
	void set_##_macro_name_(_macro_type_ arg) { _macro_name_##_ = arg; }

#endif
