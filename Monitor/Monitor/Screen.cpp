#include "stdafx.h"
#include "afxdialogex.h"
#include "Screen.h"

BEGIN_MESSAGE_MAP(Screen, CWnd)
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

Screen::Screen(pj_uint32_t index)
	: CWnd()
	, rect_(0, 0, 0, 0)
	, wrapper_(NULL)
	, index_(index)
	, uid_(0)
	, window_(NULL)
	, render_(NULL)
	, texture_(NULL)
	, active_(PJ_FALSE)
	, mutex_()
	, sync_thread_pool_(1)
	, call_status(0)
{
}

Screen::~Screen()
{
}

pj_status_t Screen::Prepare(const CRect &rect, const CWnd *wrapper, pj_uint32_t uid)
{
	pj_uint32_t width = PJ_ABS(rect.right - rect.left);
	pj_uint32_t height = PJ_ABS(rect.bottom - rect.top);

	rect_ = rect;
	wrapper_ = wrapper;
	uid_ = uid;

	pj_bool_t ret = this->Create(
		nullptr,
		nullptr,
		WS_BORDER | WS_VISIBLE | WS_CHILD,
		rect,
		(CWnd *)wrapper,
		uid_);
	pj_assert(ret == PJ_TRUE);

	window_ = SDL_CreateWindowFrom(GetSafeHwnd());
	RETURN_VAL_IF_FAIL(window_ != nullptr, PJ_EINVAL);

	render_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
	RETURN_VAL_IF_FAIL(render_ != nullptr, PJ_EINVAL);

	texture_ = SDL_CreateTexture(render_, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);
	RETURN_VAL_IF_FAIL(texture_ != nullptr, PJ_EINVAL);

	/*pj_status_t status;
	pj_uint32_t audio_ssrc = pj_rand();
	status = pjmedia_rtp_session_init(&audio_session_, RTP_MEDIA_AUDIO_TYPE, audio_ssrc);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	pj_uint32_t video_ssrc;
	do{ video_ssrc = pj_rand(); } while(video_ssrc == audio_ssrc);
	status = pjmedia_rtp_session_init(&video_session_, RTP_MEDIA_VIDEO_TYPE, video_ssrc);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);*/

	return PJ_SUCCESS;
}

pj_status_t Screen::Launch()
{

	sync_thread_pool_.Start();

	return PJ_SUCCESS;
}

void Screen::Refresh(const CRect &rect)
{
	lock_guard<std::mutex> internal_lock(mutex_);
	rect_ = rect;

	this->MoveWindow(rect);
	this->ShowWindow(SW_SHOW);
}

void Screen::Hide()
{
	lock_guard<std::mutex> internal_lock(mutex_);
	SDL_HideWindow(window_);
}

void Screen::Painting(const SDL_Rect &rect, const void *pixels, int pitch)
{
	lock_guard<std::mutex> internal_lock(mutex_);
	SDL_UpdateTexture(texture_, &rect, pixels, pitch);
	SDL_RenderClear(render_);
	SDL_RenderCopy(render_, texture_, NULL, NULL);
	SDL_RenderPresent(render_);
}

void Screen::OnRxAudio(const pj_uint8_t *rtp_frame, pj_uint16_t framelen)
{
	RETURN_IF_FAIL(framelen > 0);

	pj_uint8_t *frame_copy = (pj_uint8_t *)malloc(framelen);
	RETURN_IF_FAIL(frame_copy != nullptr);

	pj_memcpy(frame_copy, rtp_frame, framelen);

	sync_thread_pool_.Schedule([=]()
	{
		
	});
}

void Screen::OnRxVideo(const pj_uint8_t *rtp_frame, pj_uint16_t framelen)
{
	RETURN_IF_FAIL(framelen > 0);

	pj_uint8_t *frame_copy = (pj_uint8_t *)malloc(framelen);
	RETURN_IF_FAIL(frame_copy != nullptr);

	pj_memcpy(frame_copy, rtp_frame, framelen);

	pj_status_t status;
	const pjmedia_rtp_hdr *hdr;
	const void *payload;
	unsigned payloadlen;
	pjmedia_rtp_status seq_st;

	status = pjmedia_rtp_decode_rtp(&stream.dec->rtp, frame_copy, (int)framelen,
				&hdr, &payload, &payloadlen);
	if(status == PJ_SUCCESS)
	{
		pjmedia_rtp_session_update2(&stream.dec->rtp, hdr, &seq_st, PJ_TRUE);
		if (seq_st.status.flag.bad || payloadlen == 0)
			goto on_return;

		if (stream.dec->paused)
			goto on_return;

		pj_mutex_lock(stream.jb_mutex);

		if ((pj_ntohl(hdr->ts) != stream.dec_frame.timestamp.u32.lo) || hdr->m) {
			/* Only decode if we don't already have decoded one,
				* unless the jb is full.
				*/
			pj_bool_t can_decode = PJ_FALSE;

			if (pjmedia_jbuf_is_full(stream.jb)) {
				can_decode = PJ_TRUE;
			}
			else if (stream.dec_frame.size == 0) {
				can_decode = PJ_TRUE;
			}

			if (can_decode) {
				stream.dec_frame.size = 176 * 144 * 4;  // Width * Height * max_color_depth(4)
				if (decode_vid_frame() != PJ_SUCCESS) {
					stream.dec_frame.size = 0;
				}
			}
		}

		if (seq_st.status.flag.restart) {
			status = pjmedia_jbuf_reset(stream.jb);
			PJ_LOG(4,(__FILE__, "Jitter buffer reset"));
		} else {
			/* Just put the payload into jitter buffer */
			pjmedia_jbuf_put_frame3(stream.jb, payload, payloadlen, 0, 
				pj_ntohs(hdr->seq), pj_ntohl(hdr->ts), NULL);
		}
		
		pj_mutex_unlock(stream.jb_mutex);
	}

on_return :
	free(frame_copy);
}

pj_status_t Screen::decode_vid_frame()
{
    pj_uint32_t last_ts = 0;
    int frm_first_seq = 0, frm_last_seq = 0;
    pj_bool_t got_frame = PJ_FALSE;
    unsigned cnt;
    pj_status_t status;

	/* Check if we got a decodable frame */
    for (cnt=0; ; ++cnt) {
	char ptype;
	pj_uint32_t ts;
	int seq;

	/* Peek frame from jitter buffer. */
	pjmedia_jbuf_peek_frame(stream.jb, cnt, NULL, NULL,
				&ptype, NULL, &ts, &seq);
	if (ptype == PJMEDIA_JB_NORMAL_FRAME) {
	    if (last_ts == 0) {
		last_ts = ts;
		frm_first_seq = seq;
	    }
	    if (ts != last_ts) {
		got_frame = PJ_TRUE;
		break;
	    }
	    frm_last_seq = seq;
	} else if (ptype == PJMEDIA_JB_ZERO_EMPTY_FRAME) {
	    /* No more packet in the jitter buffer */
	    break;
	}
    }

    if (got_frame) {
	unsigned i;

	/* Generate frame bitstream from the payload */
	if (cnt > stream.rx_frame_cnt) {
		PJ_LOG(1,(stream.dec->port.info.name.ptr,
		      "Discarding %u frames because array is full!",
		      cnt - stream.rx_frame_cnt));
	    pjmedia_jbuf_remove_frame(stream.jb, cnt - stream.rx_frame_cnt);
	    cnt = stream.rx_frame_cnt;
	}

	for (i = 0; i < cnt; ++i) {
	    char ptype;

	    stream.rx_frames[i].type = PJMEDIA_FRAME_TYPE_VIDEO;
	    stream.rx_frames[i].timestamp.u64 = last_ts;
	    stream.rx_frames[i].bit_info = 0;

	    /* We use jbuf_peek_frame() as it will returns the pointer of
	     * the payload (no buffer and memcpy needed), just as we need.
	     */
	    pjmedia_jbuf_peek_frame(stream.jb, i,
				    (const void**)&stream.rx_frames[i].buf,
				    &stream.rx_frames[i].size, &ptype,
				    NULL, NULL, NULL);

	    if (ptype != PJMEDIA_JB_NORMAL_FRAME) {
		/* Packet lost, must set payload to NULL and keep going */
		stream.rx_frames[i].buf = NULL;
		stream.rx_frames[i].size = 0;
		stream.rx_frames[i].type = PJMEDIA_FRAME_TYPE_NONE;
		continue;
	    }
	}

	/* Decode */
	status = pjmedia_vid_codec_decode(stream.codec, cnt,
	                                  stream.rx_frames,
									  (unsigned)stream.dec_frame.size, &stream.dec_frame);
	if (status != PJ_SUCCESS) {
		stream.dec_frame.type = PJMEDIA_FRAME_TYPE_NONE;
	    stream.dec_frame.size = 0;
	}

	pjmedia_jbuf_remove_frame(stream.jb, cnt);
    }

	return PJ_SUCCESS;
}

void Screen::OnLButtonDown(UINT nFlags, CPoint point)
{
	pj_str_t remote_uri = pj_str("sip:192.168.6.40:6080");/*"sip:192.168.4.108:5060"*/
	if ( ( call_status ++ ) % 2 == 0 )
	{
		/*SessionMgr::GetInstance()->StartSession(&remote_uri, index());*/
	}
	else
	{
		/*SessionMgr::GetInstance()->StopSession(index());*/
	}
}
