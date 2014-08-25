#include "stdafx.h"
#include "afxdialogex.h"
#include "Screen.h"

BEGIN_MESSAGE_MAP(Screen, CWnd)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

Screen::Screen(pj_uint32_t index,
			   pj_sock_t &ref_tcp_sock,
			   mutex &ref_tcp_lock)
	: CWnd()
	, rect_(0, 0, 0, 0)
	, user_(nullptr)
	, wrapper_(nullptr)
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
	, ref_tcp_sock_(ref_tcp_sock)
	, ref_tcp_lock_(ref_tcp_lock)
{
}

Screen::~Screen()
{
}

pj_status_t Screen::Prepare(pj_pool_t *pool,
							const CRect &rect,
							const CWnd *wrapper,
							pj_uint32_t uid)
{
	enum { M = 32 };

	pj_uint32_t width = PJ_ABS(rect.right - rect.left);
	pj_uint32_t height = PJ_ABS(rect.bottom - rect.top);

	rect_ = rect;
	wrapper_ = wrapper;

	BOOL result;
	result = this->Create(nullptr, nullptr, WS_BORDER | WS_VISIBLE | WS_CHILD,
		rect, (CWnd *)wrapper, uid);
	RETURN_VAL_IF_FAIL(result, PJ_EINVAL);

	window_ = SDL_CreateWindowFrom(GetSafeHwnd());
	RETURN_VAL_IF_FAIL(window_ != nullptr, PJ_EINVAL);

	render_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
	RETURN_VAL_IF_FAIL(render_ != nullptr, PJ_EINVAL);

	texture_ = SDL_CreateTexture(render_, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);
	RETURN_VAL_IF_FAIL(texture_ != nullptr, PJ_EINVAL);

	/* Allocate stream */
    stream_ = PJ_POOL_ZALLOC_T(pool, vid_stream_t);
    PJ_ASSERT_RETURN(stream_ != NULL, PJ_ENOMEM);

	stream_->dec = PJ_POOL_ZALLOC_T(pool, vid_channel_t);
    PJ_ASSERT_RETURN(stream_->dec != NULL, PJ_ENOMEM);

	stream_->dec_max_size = 264 * 216 * 4;

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
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, PJ_EINVAL);

	status = pjmedia_rtp_session_init(&stream_->dec->rtp, RTP_MEDIA_AUDIO_TYPE, 0);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, PJ_EINVAL);

	status = pj_mutex_create_simple(pool, NULL, &stream_->jb_mutex);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, PJ_EINVAL);

	return PJ_SUCCESS;
}

pj_status_t Screen::Launch()
{
	audio_thread_pool_.Start();
	video_thread_pool_.Start();

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
	rect_ = rect;

	this->MoveWindow(rect);
	this->ShowWindow(SW_SHOW);
}

void Screen::HideWindow()
{
	lock_guard<std::mutex> internal_lock(render_mutex_);
	this->ShowWindow(SW_HIDE);
}

void Screen::Painting(const SDL_Rect &rect, const void *pixels, int pitch)
{
	lock_guard<std::mutex> internal_lock(render_mutex_);
	SDL_UpdateTexture(texture_, &rect, pixels, pitch);
	SDL_RenderClear(render_);
	SDL_RenderCopy(render_, texture_, NULL, NULL);
	SDL_RenderPresent(render_);
}

void Screen::AudioScene(const pj_uint8_t *rtp_frame, pj_uint16_t framelen)
{
	RETURN_IF_FAIL(media_active_);
	RETURN_IF_FAIL(framelen > 0);

	audio_thread_pool_.Schedule([=]()
	{
		vector<pj_uint8_t> frame_copy;
		frame_copy.assign(rtp_frame, rtp_frame + framelen);

		OnRxAudio(frame_copy);
	});
}

void Screen::OnRxAudio(vector<pj_uint8_t> &audio_frame)
{
}

void Screen::VideoScene(const pj_uint8_t *rtp_frame, pj_uint16_t framelen)
{
	RETURN_IF_FAIL(media_active_);
	RETURN_IF_FAIL(framelen > 0);

	video_thread_pool_.Schedule([=]()
	{
		vector<pj_uint8_t> frame_copy;
		frame_copy.assign(rtp_frame, rtp_frame + framelen);

		OnRxVideo(frame_copy);

		if (stream_->dec_frame.type == PJMEDIA_FRAME_TYPE_VIDEO &&
			stream_->dec_frame.size > 0)
		{
			// Painting();
		}
	});
}

void Screen::OnRxVideo(vector<pj_uint8_t> &video_frame)
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

		TRACE("pt:%u ssrc:%u seq:%u ts:%u payloadlen:%u\n", hdr->pt, hdr->ssrc, hdr->seq, hdr->ts, payloadlen);

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
	pjmedia_jbuf_peek_frame(stream_->jb, cnt, NULL, NULL,
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
	if (cnt > stream_->rx_frame_cnt) {
		PJ_LOG(1,(__FILE__, "Discarding %u frames because array is full!",
		      cnt - stream_->rx_frame_cnt));
	    pjmedia_jbuf_remove_frame(stream_->jb, cnt - stream_->rx_frame_cnt);
	    cnt = stream_->rx_frame_cnt;
	}

	for (i = 0; i < cnt; ++i) {
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

	    if (ptype != PJMEDIA_JB_NORMAL_FRAME) {
		/* Packet lost, must set payload to NULL and keep going */
		stream_->rx_frames[i].buf = NULL;
		stream_->rx_frames[i].size = 0;
		stream_->rx_frames[i].type = PJMEDIA_FRAME_TYPE_NONE;
		continue;
	    }
	}

	/* Decode */
	/*status = pjmedia_vid_codec_decode(stream_->codec, cnt,
	                                  stream_->rx_frames,
									  (unsigned)stream_->dec_frame.size, &stream_->dec_frame);
	if (status != PJ_SUCCESS) {
		stream_->dec_frame.type = PJMEDIA_FRAME_TYPE_NONE;
	    stream_->dec_frame.size = 0;
	}*/

	stream_->dec_frame.timestamp.u32.lo = last_ts;
	stream_->dec_frame.size = 0;

	TRACE("decode a frame success ts:%u frame_cnt:%u\n", last_ts, cnt);

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

pj_status_t Screen::LinkRoomUser(av_index_map_t &av_index_map, User *user)
{
	enum {AUDIO_INDEX, VIDEO_INDEX};

	RETURN_VAL_IF_FAIL(user, PJ_EINVAL);

	pj_status_t status;
	pj_ssize_t sndlen;
	if(user_ != nullptr)
	{
		request_to_avs_proxy_unlink_room_user_t unlink_room_user;
		unlink_room_user.client_request_type = REQUEST_FROM_CLIENT_TO_AVSPROXY_LINK_ROOM_USER;
		unlink_room_user.proxy_id = 100;
		unlink_room_user.client_id = 10;
		unlink_room_user.room_id = user_->room_id_;
		unlink_room_user.user_id = user_->user_id_;
		unlink_room_user.unlink_media_mask = MEDIA_MASK_VIDEO;
		unlink_room_user.Serialize();

		av_index_map[AUDIO_INDEX].erase(user_->audio_ssrc_);
		av_index_map[VIDEO_INDEX].erase(user_->video_ssrc_);

		sndlen = sizeof(unlink_room_user);
		status = SendTCPPacket(&unlink_room_user, &sndlen);
		RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);
	}

	media_active_ = PJ_TRUE;
	user_ = user;

	request_to_avs_proxy_link_room_user_t link_room_user;
	link_room_user.client_request_type = REQUEST_FROM_CLIENT_TO_AVSPROXY_LINK_ROOM_USER;
	link_room_user.proxy_id = 100;
	link_room_user.client_id = 10;
	link_room_user.room_id = user->room_id_;
	link_room_user.user_id = user->user_id_;
	link_room_user.link_media_mask = MEDIA_MASK_VIDEO;
	link_room_user.Serialize();

	av_index_map[AUDIO_INDEX].insert(index_map_t::value_type(user->audio_ssrc_, index_));
	av_index_map[VIDEO_INDEX].insert(index_map_t::value_type(user->video_ssrc_, index_));

	sndlen = sizeof(link_room_user);
	return SendTCPPacket(&link_room_user, &sndlen);
}

pj_status_t Screen::UnlinkRoomUser(av_index_map_t &av_index_map)
{
	RETURN_VAL_IF_FAIL(user_, PJ_EINVAL);

	request_to_avs_proxy_unlink_room_user_t unlink_room_user;
	unlink_room_user.client_request_type = REQUEST_FROM_CLIENT_TO_AVSPROXY_LINK_ROOM_USER;
	unlink_room_user.proxy_id = 100;
	unlink_room_user.client_id = 10;
	unlink_room_user.room_id = user_->room_id_;
	unlink_room_user.user_id = user_->user_id_;
	unlink_room_user.unlink_media_mask = MEDIA_MASK_VIDEO;
	unlink_room_user.Serialize();

	media_active_ = PJ_FALSE;
	user_ = nullptr;

	pj_ssize_t sndlen = sizeof(unlink_room_user);
	return SendTCPPacket(&unlink_room_user, &sndlen);
}

pj_status_t Screen::SendTCPPacket(const void *buf, pj_ssize_t *len)
{
	lock_guard<mutex> lock(ref_tcp_lock_);
	return pj_sock_send(ref_tcp_sock_, buf, len, 0);
}

void Screen::OnLButtonDown(UINT nFlags, CPoint point)
{
	pj_str_t remote_uri = pj_str("sip:192.168.6.40:6080");/*"sip:192.168.4.108:5060"*/
	if ( ( call_status_ ++ ) % 2 == 0 )
	{
		/*SessionMgr::GetInstance()->StartSession(&remote_uri, index());*/
	}
	else
	{
		/*SessionMgr::GetInstance()->StopSession(index());*/
	}
}

void Screen::OnLButtonUp(UINT nFlags, CPoint point)
{
	TRACE("Screen[%u] LButtonUp x:%ld y:%ld\n", index_, point.x, point.y);

	::SendMessage(AfxGetMainWnd()->m_hWnd, WM_ENDDRAGITEM, 0, (LPARAM)this); // let Mainframe knowns.
}
