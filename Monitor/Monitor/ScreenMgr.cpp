#include "stdafx.h"
#include "ScreenMgr.h"

extern Config g_client_config;
extern index_map_t g_av_index_map[2];
extern Screen *g_screens[MAXIMAL_SCREEN_NUM];
extern RTPSession g_rtp_session;

#ifdef __ABS_FILE__
#undef __ABS_FILE__
#endif

#define __ABS_FILE__ "ScreenMgr.cpp"

const resolution_t ScreenMgr::DEFAULT_RESOLUTION = 
{
	MININUM_TREE_CTL_WIDTH + MININUM_SCREEN_WIDTH * 3 + MININUM_PADDING,
	NIMINUM_SCREEN_HEIGHT * 3 + MININUM_PADDING
};

void ScreenMgr::event_func_proxy(evutil_socket_t fd, short event, void *arg)
{
	ev_function_t *pfunction = reinterpret_cast<ev_function_t *>(arg);
	(*pfunction)(fd, event, arg);
}

ScreenMgr::ScreenMgr(CWnd *wrapper,
					 pj_uint16_t client_id,
					 const pj_str_t &local_ip,
					 pj_uint16_t local_udp_port)
	: Noncopyable()
	, wrapper_(wrapper)
	, width_(MININUM_SCREEN_WIDTH)
	, height_(NIMINUM_SCREEN_HEIGHT)
	, screen_mgr_res_(SCREEN_RES_3x3)
	, vertical_padding_(MININUM_PADDING)
	, horizontal_padding_(MININUM_PADDING)
	, client_id_(client_id)
	, local_tcp_sock_(-1)
	, caching_pool_()
	, pool_(NULL)
	, tcp_ev_(nullptr)
	, udp_ev_(nullptr)
	, pipe_ev_(nullptr)
	, evbase_(nullptr)
	, connector_thread_()
	, event_thread_()
	, active_(PJ_FALSE)
	, titles_(nullptr)
	, screenmgr_func_array_()
	, sync_thread_pool_(1)
	, num_blocks_()
{
	screenmgr_func_array_.push_back(&ScreenMgr::ChangeLayout_1x1);
	num_blocks_.push_back(1);
	screenmgr_func_array_.push_back(&ScreenMgr::ChangeLayout_2x2);
	num_blocks_.push_back(2);
	screenmgr_func_array_.push_back(&ScreenMgr::ChangeLayout_1x5);
	num_blocks_.push_back(3);
	screenmgr_func_array_.push_back(&ScreenMgr::ChangeLayout_3x3);
	num_blocks_.push_back(3);
}

ScreenMgr::~ScreenMgr()
{
}

resolution_t ScreenMgr::GetDefaultResolution()
{
	return DEFAULT_RESOLUTION;
}

pj_status_t ScreenMgr::Prepare()
{
	pj_status_t status;

	int ret;
	ret = evutil_socketpair(AF_INET, SOCK_STREAM, 0, pipe_fds_);

	pj_caching_pool_init(&caching_pool_, &pj_pool_factory_default_policy, 0);

	pool_ = pj_pool_create(&caching_pool_.factory, "AvsProxyClientPool", 1000, 1000, NULL);

	status = log_open(pool_, g_client_config.log_file_name);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);
	PJ_LOG(5, (__ABS_FILE__, "Open log file %s success!", g_client_config.log_file_name.ptr));

	status = g_rtp_session.Open();
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);
	PJ_LOG(5, (__ABS_FILE__, "Open rtp session success!"));

	/* Init video format manager */
    status = pjmedia_video_format_mgr_create(pool_, 64, 0, NULL);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	/* Init video converter manager */
    status = pjmedia_converter_mgr_create(pool_, NULL);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

    /* Init event manager */
    status = pjmedia_event_mgr_create(pool_, 0, NULL);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	/* Init video codec manager */
    status = pjmedia_vid_codec_mgr_create(pool_, NULL);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	status = pjmedia_codec_ffmpeg_vid_init(NULL, &caching_pool_.factory);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);
	PJ_LOG(5, (__ABS_FILE__, "Initialize ffmpeg video codec success!"));

	evbase_ = event_base_new();
	RETURN_VAL_IF_FAIL(evbase_ != nullptr, PJ_EINVAL);
	
	ev_function_t function;
	ev_function_t *pfunction = nullptr;

	function = std::bind(&ScreenMgr::EventOnUdpRead, this, std::placeholders::_1, std::placeholders::_2, nullptr);
	pfunction = new ev_function_t(function);
	udp_ev_ = event_new(evbase_, g_rtp_session.GetRTPSock(), EV_READ | EV_PERSIST, event_func_proxy, pfunction);
	RETURN_VAL_IF_FAIL(udp_ev_ != nullptr, PJ_EINVAL);

	ret = event_add(udp_ev_, NULL);
	RETURN_VAL_IF_FAIL(ret == 0, PJ_EINVAL);

	function = std::bind(&ScreenMgr::EventOnPipe, this, std::placeholders::_1, std::placeholders::_2, nullptr);
	pfunction = new ev_function_t(function);
	pipe_ev_ = event_new(evbase_, pipe_fds_[0], EV_READ | EV_PERSIST, event_func_proxy, pfunction);
	RETURN_VAL_IF_FAIL(pipe_ev_ != nullptr, PJ_EINVAL);

	ret = event_add(pipe_ev_, NULL);
	RETURN_VAL_IF_FAIL(ret == 0, PJ_EINVAL);

	titles_ = new TitlesCtl();
	pj_assert(titles_ != nullptr);
	status = titles_->Prepare(wrapper_, IDC_ROOM_TREE_CTL_INDEX);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	for(pj_uint32_t idx = 0; idx < MAXIMAL_SCREEN_NUM; ++ idx)
	{
		g_screens[idx] = new Screen(idx);
		status = g_screens[idx]->Prepare(pool_, CRect(0, 0, width_, height_), wrapper_, IDC_WALL_BASE_INDEX + idx);
	}

	PJ_LOG(5, (__ABS_FILE__, "Prepare screenmgr ok!"));

	return status;
}

pj_status_t ScreenMgr::Launch()
{
	(this->* screenmgr_func_array_[GET_FUNC_INDEX(screen_mgr_res_)])(width_, height_);

	active_ = PJ_TRUE;

	event_thread_ = thread(std::bind(&ScreenMgr::EventThread, this));
	sync_thread_pool_.Start();

	for (pj_uint32_t idx = 0; idx < MAXIMAL_SCREEN_NUM; ++idx)
	{
		g_screens[idx]->Launch();
	}

	PJ_LOG(5, (__ABS_FILE__, "Launch screenmgr ok!"));

	return PJ_SUCCESS;
}

void ScreenMgr::Destory()
{
	active_ = PJ_FALSE;
	event_base_loopexit(evbase_, NULL);
	sync_thread_pool_.Stop();

	pj_sock_close(local_tcp_sock_);
}

pj_status_t ScreenMgr::OnLinkRoom(TitleRoom *title_room)
{
	vector<pj_uint8_t> response;
	http_proxy_get(g_client_config.rrtvms_fcgi_host, g_client_config.rrtvms_fcgi_port, g_client_config.rrtvms_fcgi_uri,
		title_room->id_, response);

	pj_status_t           status;
	proxy_map_t::key_type proxy_id(100);
	string                proxy_ip("192.168.6.40");
	pj_uint16_t           proxy_tcp_port(12000);
	pj_uint16_t           proxy_udp_port(13000);

	status = ParseHttpResponse(proxy_id, proxy_ip, proxy_tcp_port, proxy_udp_port, response);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	vector<pj_uint16_t> ports;
	ports.push_back(proxy_tcp_port);
	ports.push_back(proxy_udp_port);
	std::function<pj_status_t ()> connection = std::bind(&ScreenMgr::LinkRoom, this, proxy_id, proxy_ip, ports, title_room);
	std::function<pj_status_t ()> *pconnection = new std::function<pj_status_t ()>(connection);
	pj_assert(pconnection != nullptr);

	pj_ssize_t sndlen = sizeof(pconnection);
	pj_sock_send(pipe_fds_[1], &pconnection, &sndlen, 0);

	return PJ_SUCCESS;
}

pj_status_t ScreenMgr::OnUnlinkRoom(TitleRoom *title_room)
{
	std::function<pj_status_t()> disconnection = std::bind(&ScreenMgr::UnlinkRoom, this, title_room);
	std::function<pj_status_t()> *pdisconnection = new std::function<pj_status_t()>(disconnection);
	pj_assert(pdisconnection != nullptr);

	pj_ssize_t sndlen = sizeof(pdisconnection);
	pj_sock_send(pipe_fds_[1], &pdisconnection, &sndlen, 0);

	return PJ_SUCCESS;
}

// old_screen may be exist at new_user.
// old_user may be exist at new_screen
void ScreenMgr::LinkScreenUser(Screen *new_screen, User *new_user)
{
	RETURN_IF_FAIL(new_screen != nullptr);
	RETURN_IF_FAIL(new_user != nullptr);
	RETURN_IF_FAIL(new_user->title_room_ != nullptr);

	AvsProxy *proxy = new_user->title_room_->proxy_;
	RETURN_IF_FAIL(proxy != nullptr);

	if(new_user->screen_idx_ != INVALID_SCREEN_INDEX)  // 如果这个被拖拽的用户正在其他屏幕上显示
	{
		pj_assert(new_user->screen_idx_ >= 0 && new_user->screen_idx_ < MAXIMAL_SCREEN_NUM);
		Screen *old_screen = g_screens[new_user->screen_idx_];
		UnlinkScreenUser(old_screen, new_user);
	}

	User *old_user = nullptr;
	if (new_screen->GetUser(old_user) == PJ_SUCCESS)  // 如果这个屏幕上已有用户
	{
		RETURN_IF_FAIL(*new_user != *old_user);

		UnlinkScreenUser(new_screen, old_user);
	}

	g_av_index_map[AUDIO_INDEX].insert(index_map_t::value_type(new_user->audio_ssrc_, new_screen->GetIndex()));
	g_av_index_map[VIDEO_INDEX].insert(index_map_t::value_type(new_user->video_ssrc_, new_screen->GetIndex()));
	
	proxy->LinkRoomUser(new_user);
	new_screen->ConnectUser(new_user);
	new_user->ConnectScreen(new_screen->GetIndex());
}

void ScreenMgr::UnlinkScreenUser(Screen *screen, User *old_user)
{
	RETURN_IF_FAIL(screen != nullptr);
	RETURN_IF_FAIL(old_user != nullptr);
	RETURN_IF_FAIL(old_user->title_room_ != nullptr);

	screen->DisconnectUser();
	old_user->DisconnectScreen();

	g_av_index_map[AUDIO_INDEX].erase(old_user->audio_ssrc_);
	g_av_index_map[VIDEO_INDEX].erase(old_user->video_ssrc_);

	AvsProxy *proxy = old_user->title_room_->proxy_;
	RETURN_IF_FAIL(proxy != nullptr);

	proxy->UnlinkRoomUser(old_user);
}

void ScreenMgr::ChangeLayout(enum_screen_mgr_resolution_t resolution)
{
	RETURN_IF_FAIL(screen_mgr_res_ != resolution);

	screen_mgr_res_ = resolution;

	// 沿用上一次的cx,cy. 以免窗口大小被计算后越来越小.
	pj_uint32_t divisor = num_blocks_[GET_FUNC_INDEX(screen_mgr_res_)];
	pj_uint32_t width = width_, height = height_;
	pj_uint32_t round_width, round_height;
	pj_uint32_t tmp_width = width - MININUM_TREE_CTL_WIDTH;
	round_width = ROUND(tmp_width, divisor);
	round_height = ROUND(height, divisor);

	HideAll();

	(this->* screenmgr_func_array_[GET_FUNC_INDEX(screen_mgr_res_)])(round_width, round_height);
}

void ScreenMgr::GetSuitedSize(LPRECT lpRect)
{
	RETURN_IF_FAIL(active_);

	LONG screens_width, height;
	screens_width = lpRect->right - lpRect->left - 2 * SIDE_SIZE - MININUM_TREE_CTL_WIDTH;
	height = lpRect->bottom - lpRect->top - SIDE_SIZE - TOP_SIDE_SIZE;

	pj_uint32_t divisor = num_blocks_[GET_FUNC_INDEX(screen_mgr_res_)];
	ROUND(screens_width, divisor);
	ROUND(height, divisor);

	lpRect->right = lpRect->left + MININUM_TREE_CTL_WIDTH + screens_width + 2 * SIDE_SIZE;
	lpRect->bottom = lpRect->top + height + SIDE_SIZE + TOP_SIDE_SIZE;
}

void ScreenMgr::Adjest(pj_int32_t &cx, pj_int32_t &cy)
{
	RETURN_IF_FAIL(active_);

	width_ = cx;
	height_ = cy;

	pj_uint32_t divisor = num_blocks_[GET_FUNC_INDEX(screen_mgr_res_)];
	pj_uint32_t round_width, round_height;
	pj_uint32_t tmp_width = cx - MININUM_TREE_CTL_WIDTH;
	round_width  = ROUND(tmp_width, divisor);
	round_height = ROUND(cy, divisor);

	(this->* screenmgr_func_array_[GET_FUNC_INDEX(screen_mgr_res_)])(round_width, round_height);
}

void ScreenMgr::ChangeLayout_1x1(pj_uint32_t width, pj_uint32_t height)
{
	CRect rect(0, 0, MININUM_TREE_CTL_WIDTH, height * num_blocks_[0]);
	titles_->MoveToRect(rect);

	g_screens[0]->MoveToRect(CRect(MININUM_TREE_CTL_WIDTH, 0, MININUM_TREE_CTL_WIDTH + width, height));
}

void ScreenMgr::ChangeLayout_2x2(pj_uint32_t width, pj_uint32_t height)
{
	CRect rect(0, 0, MININUM_TREE_CTL_WIDTH, height * num_blocks_[1]);
	titles_->MoveToRect(rect);

	pj_uint32_t lstart = MININUM_TREE_CTL_WIDTH;

	const unsigned MAX_COL = 2, MAX_ROW = 2;
	for ( unsigned row = 0; row < MAX_COL; ++ row )
	{
		for ( unsigned col = 0; col < MAX_ROW; ++ col )
		{
			unsigned idx = col + row * MAX_COL;
			CRect rect;
			rect.left  = col * width + col * horizontal_padding_ + lstart;
			rect.top   = row * height + row * vertical_padding_;
			rect.right = rect.left + width;
			rect.bottom = rect.top + height;

			g_screens[idx]->MoveToRect(rect);
		}
	}
}

void ScreenMgr::ChangeLayout_1x5(pj_uint32_t width, pj_uint32_t height)
{
	CRect rect(0, 0, MININUM_TREE_CTL_WIDTH, height * num_blocks_[2]);
	titles_->MoveToRect(rect);

	pj_uint32_t lstart = MININUM_TREE_CTL_WIDTH;
	unsigned idx = 0;

	pj_int32_t left = lstart, top = 0, right, bottom;
	right = left + width * 2 + horizontal_padding_;
	bottom = height * 2 + vertical_padding_;
	g_screens[idx++]->MoveToRect(CRect(left, top, right, bottom));

	left = right + horizontal_padding_;
	right = left + width;
	bottom = height;
	g_screens[idx++]->MoveToRect(CRect(left, top, right, bottom));

	top = bottom + vertical_padding_;
	bottom = top + height;
	g_screens[idx++]->MoveToRect(CRect(left, top, right, bottom));

	left = lstart;
	right = left + width;
	top = bottom + vertical_padding_;
	bottom = top + height;
	g_screens[idx++]->MoveToRect(CRect(left, top, right, bottom));

	left = right + horizontal_padding_;
	right = left + width;
	g_screens[idx++]->MoveToRect(CRect(left, top, right, bottom));

	left = right + horizontal_padding_;
	right = left + width;
	g_screens[idx++]->MoveToRect(CRect(left, top, right, bottom));
}

void ScreenMgr::ChangeLayout_3x3(pj_uint32_t width, pj_uint32_t height)
{
	CRect rect(0, 0, MININUM_TREE_CTL_WIDTH, height * num_blocks_[3]);
	titles_->MoveToRect(rect);

	pj_uint32_t lstart = MININUM_TREE_CTL_WIDTH;

	const unsigned MAX_COL = 3, MAX_ROW = 3;
	for ( unsigned row = 0; row < MAX_COL; ++ row )
	{
		for ( unsigned col = 0; col < MAX_ROW; ++ col )
		{
			unsigned idx = col + row * MAX_COL;
			CRect rect;
			rect.left  = col * width + col * horizontal_padding_ + lstart;
			rect.top   = row * height + row * vertical_padding_;
			rect.right = rect.left + width;
			rect.bottom = rect.top + height;

			g_screens[idx]->MoveToRect(rect);
		}
	}
}

void ScreenMgr::HideAll()
{
	for (pj_uint8_t idx = 0; idx < MAXIMAL_SCREEN_NUM; ++idx)
	{
		g_screens[idx]->HideWindow();
	}
}

pj_status_t ScreenMgr::ParseHttpResponse(pj_uint16_t &proxy_id, string &proxy_ip, pj_uint16_t &proxy_tcp_port, pj_uint16_t &proxy_udp_port,
										 const vector<pj_uint8_t> &response)
{
	RETURN_VAL_IF_FAIL(response.size() > 0, PJ_EINVAL);

	enum {PROXY_ID = 0, PROXY_IP, PROXY_TCP_PORT, PROXY_UDP_PORT, TOKEN_SIZE};
	const char *delim = "\n";
	int i = 0;

	char *tmp = (char *)&response[0];
	char *s, *next = nullptr;
	s = strtok_s(tmp, delim, &next);
	while(s && i < TOKEN_SIZE)
	{
		switch(i ++)
		{
			case PROXY_ID:
				proxy_id = atoi(s);
				break;
			case PROXY_IP:
				proxy_ip.assign(s);
				break;
			case PROXY_TCP_PORT:
				proxy_tcp_port = atoi(s);
				break;
			case PROXY_UDP_PORT:
				proxy_udp_port = atoi(s);
				break;
		}
		s = strtok_s(nullptr, delim, &next);
	}

	return i == TOKEN_SIZE ? PJ_SUCCESS : PJ_EINVAL;
}

void ScreenMgr::TcpParamScene(const pj_uint8_t *storage,
							  pj_uint16_t storage_len)
{
	RETURN_IF_FAIL(storage && (storage_len > 0));

	TcpParameter *param = nullptr;
	TcpScene     *scene = nullptr;
	pj_uint16_t   type = (pj_uint16_t)ntohs(*(pj_uint16_t *)(storage + sizeof(pj_uint16_t)));

	switch(type)
	{
		case RESPONSE_FROM_AVSPROXY_TO_CLIENT_LOGIN:
		{
			param = new ResLoginParameter(storage, storage_len);
			scene = new ResLoginScene();
			break;
		}
		case REQUEST_FROM_AVSPROXY_TO_CLIENT_ROOMS_INFO:
		{
			param = new RoomsInfoParameter(storage, storage_len);
			scene = new RoomsInfoScene();
			break;
		}
		case REQUEST_FROM_AVSPROXY_TO_CLIENT_ROOM_MOD_MEDIA:
		{
			param = new ModMediaParameter(storage, storage_len);
			scene = new ModMediaScene();
		    break;
		}
		case REQUEST_FROM_AVSPROXY_TO_CLIENT_ROOM_ADD_USER:
		{
			param = new AddUserParameter(storage, storage_len);
			scene = new AddUserScene();
			break;
		}
		case REQUEST_FROM_AVSPROXY_TO_CLIENT_ROOM_DEL_USER:
		{
			param = new DelUserParameter(storage, storage_len);
			scene = new DelUserScene();
			break;
		}
		case RESPONSE_FROM_AVSPROXY_TO_CLIENT_KEEP_ALIVE:
		{
			param = new KeepAliveParameter(storage, storage_len);
			scene = new KeepAliveScene();
			break;
		}
		default:
		{
			PJ_LOG(5, ("ScreenMgr", "Tcp type:%u is invalid!! Please check it!!", type));
			return;
		}
	}

	AvsProxy *proxy = nullptr;
	RETURN_IF_FAIL(GetProxy(param->proxy_id_, proxy) == PJ_SUCCESS);
	RETURN_IF_FAIL(param != nullptr && scene != nullptr);

	sync_thread_pool_.Schedule(std::bind(&TcpScene::Maintain, shared_ptr<TcpScene>(scene), shared_ptr<TcpParameter>(param), proxy));
}

void ScreenMgr::UdpParamScene(const pjmedia_rtp_hdr *rtp_hdr,
							  const pj_uint8_t *storage,
							  pj_uint16_t storage_len)
{
	RETURN_IF_FAIL(rtp_hdr && storage && storage_len > 0);
	
	if(rtp_hdr->pt == RTP_EXPAND_PAYLOAD_TYPE)
	{
		UdpParameter *param = new NATParameter(storage, storage_len);
		UdpScene     *scene = new NATScene();

		AvsProxy *proxy = nullptr;
		RETURN_IF_FAIL(GetProxy(param->proxy_id_, proxy) == PJ_SUCCESS);
		RETURN_IF_FAIL(param != nullptr && scene != nullptr);

		sync_thread_pool_.Schedule(std::bind(&UdpScene::Maintain, shared_ptr<UdpScene>(scene), shared_ptr<UdpParameter>(param), proxy));
	}
	else
	{
		const pj_uint8_t media_index = (rtp_hdr->pt == RTP_MEDIA_VIDEO_TYPE) ? VIDEO_INDEX : 
			(rtp_hdr->pt == RTP_MEDIA_AUDIO_TYPE ? AUDIO_INDEX : -1);
		RETURN_IF_FAIL(media_index != -1);

		Screen *screen = nullptr;
		index_map_t::iterator pscreen_idx = g_av_index_map[media_index].find(rtp_hdr->ssrc);
		if (pscreen_idx != g_av_index_map[media_index].end())
		{
			index_map_t::mapped_type screen_idx = pscreen_idx->second;
			if (screen_idx >= 0 && screen_idx < MAXIMAL_SCREEN_NUM)
			{
				screen = g_screens[screen_idx];
			}
		}

		RETURN_IF_FAIL(screen != nullptr);

		media_index == AUDIO_INDEX ?
			screen->AudioScene(storage, storage_len) :
			screen->VideoScene(storage, storage_len);
	}
}

void ScreenMgr::EventOnTcpRead(evutil_socket_t fd, short event, void *arg)
{
	pj_status_t status;

	proxy_map_t::mapped_type proxy = reinterpret_cast<proxy_map_t::mapped_type>(arg);
	RETURN_IF_FAIL(proxy != nullptr);
	RETURN_IF_FAIL(event & EV_READ);

	pj_ssize_t recvlen = MAX_STORAGE_SIZE - proxy->tcp_storage_offset_;
	status = pj_sock_recv(proxy->sock_,
		(char *)(proxy->tcp_storage_ + proxy->tcp_storage_offset_),
		&recvlen,
		0);

	if (recvlen > 0)
	{
		proxy->tcp_storage_offset_ += (pj_uint16_t)recvlen;
		do {
			pj_uint16_t packet_len = ntohs(*(pj_uint16_t *)proxy->tcp_storage_);
			pj_uint16_t total_len = packet_len + sizeof(packet_len);

			if (total_len > MAX_STORAGE_SIZE)
			{
				proxy->tcp_storage_offset_ = 0;
				break;
			}
			else if (total_len > proxy->tcp_storage_offset_)
			{
				break;
			}
			else if (total_len <= proxy->tcp_storage_offset_)
			{
				proxy->tcp_storage_offset_ -= total_len;

				TcpParamScene(proxy->tcp_storage_, total_len);

				if (proxy->tcp_storage_offset_ > 0)
				{
					memcpy(proxy->tcp_storage_,
						proxy->tcp_storage_ + total_len,
						proxy->tcp_storage_offset_);
				}
				else if(proxy->tcp_storage_offset_ == 0)
				{
					break;
				}
			}
		} while (1);
	}
	else if (recvlen <= 0)
	{
		pj_sock_close(proxy->sock_);
		event_del(proxy->tcp_ev_);
		event_free(proxy->tcp_ev_);

		PJ_LOG(5, (__ABS_FILE__, "EventOnTcpRead() => Proxy was disconnected, code %d", recvlen));

		DelProxy(proxy);
	}
}

void ScreenMgr::EventOnUdpRead(evutil_socket_t fd, short event, void *arg)
{
	RETURN_IF_FAIL(event & EV_READ);

	pj_uint8_t datagram[MAX_UDP_DATA_SIZE];
	pj_ssize_t datalen = MAX_UDP_DATA_SIZE;
	pj_sock_t local_udp_sock = fd;
	const pjmedia_rtp_hdr *rtp_hdr;

	const pj_uint8_t *payload;
    unsigned payload_len;

	pj_sockaddr_in addr;
	int addrlen = sizeof(addr);

	pj_status_t status;
	status = pj_sock_recvfrom(g_rtp_session.GetRTPSock(), datagram, &datalen, 0, &addr, &addrlen);
	RETURN_IF_FAIL(status == PJ_SUCCESS);

	if (datalen >= sizeof(*rtp_hdr)
		&& datalen < (1 << 16))  // max data size is 2^16
	{
		status = pjmedia_rtp_decode_rtp(NULL,
			datagram, (int)datalen,
			&rtp_hdr, (const void **)&payload, &payload_len);
		RETURN_IF_FAIL(status == PJ_SUCCESS);

		if(rtp_hdr->pt == RTP_EXPAND_PAYLOAD_TYPE)
		{
			UdpParamScene(rtp_hdr, payload, (pj_uint16_t)payload_len);
		}
		else
		{
			UdpParamScene(rtp_hdr, datagram, (pj_uint16_t)datalen);
		}
	}
}

void ScreenMgr::EventOnPipe(evutil_socket_t fd, short event, void *arg)
{
	std::function<pj_status_t ()> *pconnection = nullptr;
	RETURN_IF_FAIL(event & EV_READ);

	pj_ssize_t recvlen = sizeof(pconnection);
	pj_status_t status;
	status = pj_sock_recv(pipe_fds_[0], &pconnection, &recvlen, 0);

	pj_assert(recvlen == sizeof(pconnection));
	pj_assert(pconnection != nullptr);

	status = (*pconnection)();
	delete pconnection;
}

pj_status_t ScreenMgr::LinkRoom(pj_uint16_t id, string ip, vector<pj_uint16_t> ports, TitleRoom *title_room)
{
	proxy_map_t::mapped_type proxy = nullptr;
	pj_status_t status;
	status = GetProxy(id, proxy);
	
	if(status != PJ_SUCCESS && ports.size() == 2) // Proxy isn't exist!
	{
		pj_sock_t sock;
		pj_str_t pj_ip = pj_str((char *)ip.c_str());
		status = pj_open_tcp_clientport(&pj_ip, ports[0], sock);
		RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status); // 连不上proxy

		status = AddProxy(id, pj_ip, ports[0], ports[1], sock, proxy);
		RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

		ev_function_t function;
		ev_function_t *pfunction = nullptr;

		function = std::bind(&ScreenMgr::EventOnTcpRead, this, std::placeholders::_1, std::placeholders::_2, proxy);
		pfunction = new ev_function_t(function);
		proxy->pfunction_ = pfunction;

		proxy->tcp_ev_ = event_new(evbase_, proxy->sock_, EV_READ | EV_PERSIST, event_func_proxy, pfunction);
		RETURN_VAL_WITH_STATEMENT_IF_FAIL(proxy->tcp_ev_ != nullptr,
			(linked_proxys_.erase(id), delete proxy, delete pfunction, pj_sock_close(sock)),
			PJ_EINVAL);

		RETURN_VAL_WITH_STATEMENT_IF_FAIL(event_add(proxy->tcp_ev_, NULL) == 0,
			(linked_proxys_.erase(id), delete proxy, delete pfunction, pj_sock_close(sock)),
			PJ_EINVAL);

		status = proxy->Login();
		RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);
	}

	status = proxy->LinkRoom(title_room);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS || status == PJ_EEXISTS, status);

	return PJ_SUCCESS;
}

pj_status_t ScreenMgr::UnlinkRoom(TitleRoom *title_room)
{
	RETURN_VAL_IF_FAIL(title_room != nullptr, PJ_EINVAL);

	proxy_map_t::mapped_type proxy = title_room->proxy_;
	RETURN_VAL_IF_FAIL(proxy != nullptr, PJ_EINVAL);

	pj_status_t status;
	status = proxy->UnlinkRoom(title_room);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	if (proxy->GetRoomSize() == 0)
	{
		pj_sock_close(proxy->sock_);  // 当不关注此Proxy的所有房间时, 主动断开连接
		event_del(proxy->tcp_ev_);
		event_free(proxy->tcp_ev_);

		status = DelProxy(proxy);
		RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);
	}

	return PJ_SUCCESS;
}

pj_status_t ScreenMgr::AddProxy(pj_uint16_t id, pj_str_t &ip, pj_uint16_t tcp_port, pj_uint16_t udp_port, pj_sock_t sock, proxy_map_t::mapped_type &proxy)
{
	lock_guard<mutex> lock(linked_proxys_lock_);
	proxy_map_t::iterator pproxy = linked_proxys_.find(id);
	RETURN_VAL_IF_FAIL(pproxy == linked_proxys_.end(), PJ_EINVAL);

	proxy = new AvsProxy(id, ip, tcp_port, udp_port, sock);
	pj_assert(proxy != nullptr);
	linked_proxys_.insert(proxy_map_t::value_type(id, proxy));

	return PJ_SUCCESS;
}

pj_status_t ScreenMgr::DelProxy(proxy_map_t::mapped_type proxy)
{
	RETURN_VAL_IF_FAIL( proxy != nullptr, PJ_SUCCESS );

	lock_guard<mutex> lock(linked_proxys_lock_);
	proxy_map_t::iterator pproxy = linked_proxys_.find(proxy->id_);
	RETURN_VAL_IF_FAIL(pproxy != linked_proxys_.end(), PJ_EINVAL);

	linked_proxys_.erase(pproxy);

	/**< Prevent using termination before delete it.*/
	DiscProxyScene *scene = new DiscProxyScene();
	sync_thread_pool_.Schedule(std::bind(&DiscProxyScene::Maintain, shared_ptr<DiscProxyScene>(scene), proxy));

	return PJ_SUCCESS;
}

pj_status_t ScreenMgr::GetProxy(pj_uint16_t id, proxy_map_t::mapped_type &proxy)
{
	lock_guard<mutex> lock(linked_proxys_lock_);
	proxy_map_t::iterator pproxy = linked_proxys_.find(id);
	RETURN_VAL_IF_FAIL(pproxy != linked_proxys_.end(), PJ_EINVAL);

	proxy = pproxy->second;
	RETURN_VAL_IF_FAIL(proxy != nullptr, PJ_EINVAL);

	return PJ_SUCCESS;
}

void ScreenMgr::EventThread()
{
	pj_thread_desc rtpdesc;
	pj_thread_t *thread = 0;
	
	if ( !pj_thread_is_registered() )
	{
		if ( pj_thread_register(NULL, rtpdesc, &thread) == PJ_SUCCESS )
		{
			while ( active_ )
			{
				event_base_loop(evbase_, EVLOOP_ONCE);
			}
		}
	}
}

pj_status_t ScreenMgr::SendTCPPacket(const void *buf, pj_ssize_t *len)
{
	lock_guard<mutex> lock(local_tcp_lock_);
	return pj_sock_send(local_tcp_sock_, buf, len, 0);
}
