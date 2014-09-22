#include "stdafx.h"
#include "afxdialogex.h"
#include "Screen.h"

#ifdef __ABS_FILE__
#undef __ABS_FILE__
#endif

#define __ABS_FILE__ "Screen.cpp"

BEGIN_MESSAGE_MAP(Screen, CWnd)
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()

Screen *g_screens[MAXIMAL_SCREEN_NUM];

Screen::Screen(pj_uint32_t index)
	: CWnd()
	, user_(nullptr)
	, index_(index)
	, window_(nullptr)
	, render_(nullptr)
	, texture_(nullptr)
	, render_mutex_()
	, audio_thread_pool_(1)
	, video_thread_pool_(1)
	, media_active_(PJ_FALSE)
	, call_status_(0)
	, stream_(nullptr)
{
}

Screen::~Screen()
{
}

#define WIDTH  320
#define HEIGHT 240

pj_status_t Screen::Prepare(pj_pool_t *pool,
							const CRect &rect,
							const CWnd *wrapper,
							pj_uint32_t uid)
{
	enum { M = 32 };

	BOOL result;
	result = Create(nullptr, nullptr, WS_VISIBLE | WS_TABSTOP | WS_CHILD | WS_BORDER
		| TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES,
		rect, (CWnd *)wrapper, uid);
	RETURN_VAL_IF_FAIL(result, PJ_EINVAL);

	window_ = SDL_CreateWindowFrom(GetSafeHwnd());
	RETURN_VAL_IF_FAIL(window_ != nullptr, PJ_EINVAL);

	render_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
	RETURN_VAL_IF_FAIL(render_ != nullptr, PJ_EINVAL);

	texture_ = SDL_CreateTexture(render_, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
	RETURN_VAL_IF_FAIL(texture_ != nullptr, PJ_EINVAL);

	/* Allocate stream */
    stream_ = PJ_POOL_ZALLOC_T(pool, vid_stream_t);
    PJ_ASSERT_RETURN(stream_ != NULL, PJ_ENOMEM);

	stream_->dec = PJ_POOL_ZALLOC_T(pool, vid_channel_t);
    PJ_ASSERT_RETURN(stream_->dec != NULL, PJ_ENOMEM);

	stream_->dec_max_size = WIDTH * HEIGHT * 4;
	stream_->dec_frame.buf = pj_pool_alloc(pool, stream_->dec_max_size);

	unsigned chunks_per_frm = PJMEDIA_MAX_VIDEO_ENC_FRAME_SIZE / PJMEDIA_MAX_MRU;
	int frm_ptime = 1000 * 1 / 25;
	unsigned jb_max = 500 * chunks_per_frm / frm_ptime;

	stream_->rx_frame_cnt = chunks_per_frm * 2;
    stream_->rx_frames = (pjmedia_frame *)pj_pool_calloc(pool, stream_->rx_frame_cnt,
                                       sizeof(stream_->rx_frames[0]));
	pj_str_t name;
	name.ptr = (char*) pj_pool_alloc(pool, M);
    name.slen = pj_ansi_snprintf(name.ptr, M, "%s%p", "vstenc", stream_);

	pj_status_t status;
	status = pjmedia_jbuf_create(pool, &name,
		PJMEDIA_MAX_MRU,
		1000 * 1 / 25,
		jb_max, &stream_->jb);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	status = pjmedia_rtp_session_init(&stream_->dec->rtp, RTP_MEDIA_VIDEO_TYPE, 0);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	status = pj_mutex_create_simple(pool, NULL, &stream_->jb_mutex);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	pjmedia_vid_codec_mgr *codec_mgr = pjmedia_vid_codec_mgr_instance();

	pj_str_t h264_id = pj_str("H264");
	unsigned info_cnt;
	const pjmedia_vid_codec_info *codec_info;
	status = pjmedia_vid_codec_mgr_find_codecs_by_id(codec_mgr,
		&h264_id, 
		&info_cnt, 
		&codec_info,
		NULL);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	status = pjmedia_vid_codec_mgr_alloc_codec(codec_mgr, 
		codec_info,
		&stream_->codec);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	 /* Init and open the codec. */
    status = pjmedia_vid_codec_init(stream_->codec, pool);
    RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	pjmedia_vid_codec_param info_param;
	status = pjmedia_vid_codec_mgr_get_default_param(codec_mgr,
		codec_info,
		&info_param);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

    status = pjmedia_vid_codec_open(stream_->codec, &info_param);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	PJ_LOG(5, (__ABS_FILE__, "Prepare screen index[%u] size[%dx%d] ok!", index_, WIDTH, HEIGHT));

	return PJ_SUCCESS;
}

pj_status_t Screen::Launch()
{
	audio_thread_pool_.Start();
	video_thread_pool_.Start();

	PJ_LOG(5, (__ABS_FILE__, "Launch screen index[%u] ok!", index_));

	return PJ_SUCCESS;
}

void Screen::Destory()
{
	audio_thread_pool_.Stop();
	video_thread_pool_.Stop();
}

void Screen::MoveToRect(const CRect &rect)
{
	lock_guard<std::mutex> internal_lock(render_mutex_);
	MoveWindow(rect);
	ShowWindow(SW_SHOW);
}

void Screen::HideWindow()
{
	lock_guard<std::mutex> internal_lock(render_mutex_);
	ShowWindow(SW_HIDE);
}

void Screen::UpdateWindow()
{
	lock_guard<std::mutex> internal_lock(render_mutex_);
	CWnd::UpdateWindow();
}

void Screen::Painting(const void *pixels)
{
	lock_guard<std::mutex> internal_lock(render_mutex_);
	int pitch = WIDTH * SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_IYUV);
	SDL_Rect sdl_rect = {0, 0, WIDTH, HEIGHT};

	SDL_UpdateTexture(texture_, &sdl_rect, pixels, pixels != nullptr ? pitch : 0);
	SDL_RenderClear(render_);
	SDL_RenderCopy(render_, texture_, NULL, NULL);
	SDL_RenderPresent(render_);
}

void Screen::AudioScene(const pj_uint8_t *rtp_frame, pj_uint16_t framelen)
{
	{
		lock_guard<mutex> lock(media_active_lock_);
		RETURN_IF_FAIL(media_active_);
	}
	RETURN_IF_FAIL(rtp_frame && framelen > 0);

	audio_thread_pool_.Schedule(std::bind(&Screen::OnRxAudio, this, vector<pj_uint8_t>(rtp_frame, rtp_frame + framelen)));
}

void Screen::OnRxAudio(const vector<pj_uint8_t> &audio_frame)
{
	RETURN_IF_FAIL(audio_frame.size() > 0);
}

void Screen::VideoScene(const pj_uint8_t *rtp_frame, pj_uint16_t framelen)
{
	{
		lock_guard<mutex> lock(media_active_lock_);
		RETURN_IF_FAIL(media_active_);
	}
	RETURN_IF_FAIL(rtp_frame && framelen > 0);

	video_thread_pool_.Schedule(std::bind(&Screen::OnRxVideo, this, vector<pj_uint8_t>(rtp_frame, rtp_frame + framelen)));
}

void Screen::OnRxVideo(const vector<pj_uint8_t> &video_frame)
{
	RETURN_IF_FAIL(video_frame.size() > 0);

	pj_status_t status;
	const pjmedia_rtp_hdr *hdr;
	const void *payload;
	unsigned payloadlen;
	pjmedia_rtp_status seq_st;

	status = pjmedia_rtp_decode_rtp(&stream_->dec->rtp, &video_frame[0], (int)video_frame.size(),
				&hdr, &payload, &payloadlen);
	if(status == PJ_SUCCESS)
	{
		pjmedia_rtp_session_update2(&stream_->dec->rtp, hdr, &seq_st, PJ_TRUE);
		if (payloadlen == 0)
			return;

		pj_mutex_lock(stream_->jb_mutex);

		if ((pj_ntohl(hdr->ts) != stream_->dec_frame.timestamp.u32.lo) || hdr->m) {
			/* Only decode if we don't already have decoded one,
				* unless the jb is full.
				*/
			pj_bool_t can_decode = PJ_FALSE;

			if (pjmedia_jbuf_is_full(stream_->jb)) {
				can_decode = PJ_TRUE;
			}
			else if (stream_->dec_frame.size == 0) {
				can_decode = PJ_TRUE;
			}

			if (can_decode) {
				stream_->dec_frame.size = stream_->dec_max_size;  // Width * Height * max_color_depth(4)
				if (decode_vid_frame() != PJ_SUCCESS) {
					stream_->dec_frame.size = 0;
				}
			}
		}

		if (seq_st.status.flag.restart) {
			status = pjmedia_jbuf_reset(stream_->jb);
			PJ_LOG(4,(__FILE__, "Jitter buffer reset"));
		} else {
			/* Just put the payload into jitter buffer */
			pjmedia_jbuf_put_frame3(stream_->jb, payload, payloadlen, 0, 
				hdr->seq, hdr->ts, NULL);
		}

		pj_mutex_unlock(stream_->jb_mutex);
	}

	if (stream_->dec_frame.type == PJMEDIA_FRAME_TYPE_VIDEO
		&& stream_->dec_frame.size > 0)
	{
		Painting(stream_->dec_frame.buf);
		stream_->dec_frame.size = 0;
	}
}

pj_status_t Screen::decode_vid_frame()
{
    pj_uint32_t last_ts = 0;
    int frm_first_seq = 0, frm_last_seq = 0;
    pj_bool_t got_frame = PJ_FALSE;
    unsigned cnt;
    pj_status_t status;

	/* Check if we got a decodable frame */
    for (cnt=0; ; ++cnt)
	{
		char ptype;
		pj_uint32_t ts;
		int seq;

		/* Peek frame from jitter buffer. */
		pjmedia_jbuf_peek_frame(stream_->jb, cnt, NULL, NULL,
					&ptype, NULL, &ts, &seq);
		if (ptype == PJMEDIA_JB_NORMAL_FRAME)
		{
			if (last_ts == 0)
			{
				last_ts = ts;
				frm_first_seq = seq;
			}
			if (ts != last_ts)
			{
				got_frame = PJ_TRUE;
				break;
			}
			frm_last_seq = seq;
		}
		else if (ptype == PJMEDIA_JB_ZERO_EMPTY_FRAME)
		{
			/* No more packet in the jitter buffer */
			break;
		}
    }

    if (got_frame)
	{
		unsigned i;

		/* Generate frame bitstream from the payload */
		if (cnt > stream_->rx_frame_cnt)
		{
			PJ_LOG(1,(__FILE__, "Discarding %u frames because array is full!",
				cnt - stream_->rx_frame_cnt));
			pjmedia_jbuf_remove_frame(stream_->jb, cnt - stream_->rx_frame_cnt);
			cnt = stream_->rx_frame_cnt;
		}

		for (i = 0; i < cnt; ++i)
		{
			char ptype;

			stream_->rx_frames[i].type = PJMEDIA_FRAME_TYPE_VIDEO;
			stream_->rx_frames[i].timestamp.u64 = last_ts;
			stream_->rx_frames[i].bit_info = 0;

			/* We use jbuf_peek_frame() as it will returns the pointer of
			 * the payload (no buffer and memcpy needed), just as we need.
			 */
			pjmedia_jbuf_peek_frame(stream_->jb, i,
				(const void**)&stream_->rx_frames[i].buf,
				&stream_->rx_frames[i].size, &ptype,
				NULL, NULL, NULL);

			if (ptype != PJMEDIA_JB_NORMAL_FRAME)
			{
				/* Packet lost, must set payload to NULL and keep going */
				stream_->rx_frames[i].buf = NULL;
				stream_->rx_frames[i].size = 0;
				stream_->rx_frames[i].type = PJMEDIA_FRAME_TYPE_NONE;
				continue;
			}
		}

		/* Decode */
		status = pjmedia_vid_codec_decode(stream_->codec, cnt,
			stream_->rx_frames,
			(unsigned)stream_->dec_frame.size, &stream_->dec_frame);
		if (status != PJ_SUCCESS)
		{
			stream_->dec_frame.type = PJMEDIA_FRAME_TYPE_NONE;
			stream_->dec_frame.size = 0;
		}

		pjmedia_jbuf_remove_frame(stream_->jb, cnt);
    }

	/* Learn remote frame rate after successful decoding */
    if (stream_->dec_frame.type == PJMEDIA_FRAME_TYPE_VIDEO
		&& stream_->dec_frame.size)
    {
		stream_->last_dec_seq = frm_last_seq;
		stream_->last_dec_ts = last_ts;
	}

	return got_frame ? PJ_SUCCESS : PJ_ENOTFOUND;
}

pj_status_t Screen::GetUser(User *&user)
{
	return (user = const_cast<User *>(user_)) != nullptr ? PJ_SUCCESS : PJ_ENOTFOUND;
}

pj_status_t Screen::ConnectUser(User *user)
{
	RETURN_VAL_IF_FAIL(user, PJ_EINVAL);

	{
		lock_guard<mutex> lock(media_active_lock_);
		media_active_ = PJ_TRUE;
	}
	user_ = user;

	PJ_LOG(5, (__ABS_FILE__, "screen[%u] was connected to new user[%ld]", index_, user->user_id_));

	return PJ_SUCCESS;
}

pj_status_t Screen::DisconnectUser()
{
	RETURN_VAL_IF_FAIL(user_, PJ_EINVAL);

	PJ_LOG(5, (__ABS_FILE__, "screen[%u] was disconnected. Old user[%ld]", index_, user_->user_id_));

	{
		lock_guard<mutex> lock(media_active_lock_);
		media_active_ = PJ_FALSE;
	}
	user_ = nullptr;
	this->UpdateWindow();

	return PJ_SUCCESS;
}

static Screen *old_screen = nullptr;
void Screen::OnMouseMove(UINT nFlags, CPoint point)
{
	if(user_ != nullptr)
	{
		if (!g_TrackingMouse)
		{
			TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT)};
			tme.hwndTrack = m_hWnd;
			tme.dwFlags = TME_LEAVE;

			TrackMouseEvent(&tme);

			::SendMessage(g_hwndTrackingTT, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&g_toolItem);

			g_TrackingMouse = TRUE; 
		}

		if(old_screen != this)
		{
			old_screen = this;

			TitleRoom *title_room = user_->title_room_;
			if(title_room != nullptr)
			{
				WCHAR coords[128];
				swprintf_s(coords, ARRAYSIZE(coords), _T("所属房间ID: %d 用户ID: %ld"), title_room->id_, user_->user_id_);
				g_toolItem.lpszText = coords;
				::SendMessage(g_hwndTrackingTT, TTM_SETTOOLINFO, 0, (LPARAM)&g_toolItem);

				POINT pt = {point.x, point.y};
				::ClientToScreen(m_hWnd, &pt);
				::SendMessage(g_hwndTrackingTT, TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(pt.x + 10, pt.y));
			}
		}
	}

	if(user_ != nullptr)
	{
		POINT pt = {point.x, point.y};
		::ClientToScreen(m_hWnd, &pt);
		::SendMessage(g_hwndTrackingTT, TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(pt.x + 10, pt.y));
	}

	return CWnd::OnMouseMove(nFlags, point);
}

void Screen::OnMouseLeave()
{
	if(g_TrackingMouse)
	{
		::SendMessage(g_hwndTrackingTT, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)&g_toolItem);
		g_TrackingMouse = FALSE;
	}
	old_screen = nullptr;
	
	CWnd::OnMouseLeave();
}

void Screen::OnLButtonUp(UINT nFlags, CPoint point)
{
	::SendMessage(AfxGetMainWnd()->m_hWnd, WM_LINK_ROOM_USER, 0, (LPARAM)this); // let Mainframe knowns.
}

void Screen::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	::SendMessage(AfxGetMainWnd()->m_hWnd, WM_UNLINK_ROOM_USER, (WPARAM)user_, (LPARAM)this);
}
