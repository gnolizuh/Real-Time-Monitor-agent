#include "stdafx.h"
#include "ScreenMgr.h"

const resolution_t ScreenMgr::DEFAULT_RESOLUTION = 
{
	MININUM_TREE_CTL_WIDTH + MININUM_SCREEN_WIDTH * 3 + MININUM_PADDING,
	NIMINUM_SCREEN_HEIGHT * 3 + MININUM_PADDING
};

void ScreenMgr::event_on_tcp_read(evutil_socket_t fd, short event, void *arg)
{
	ScreenMgr *mgr = static_cast<ScreenMgr *>(arg);
	mgr->EventOnTcpRead(fd, event);
}

void ScreenMgr::event_on_udp_read(evutil_socket_t fd, short event, void *arg)
{
	ScreenMgr *mgr = static_cast<ScreenMgr *>(arg);
	mgr->EventOnUdpRead(fd, event);
}

ScreenMgr::ScreenMgr(CWnd *wrapper,
					 pj_uint16_t avsproxy_id,
					 const pj_str_t &avsproxy_ip,
					 pj_uint16_t avsproxy_tcp_port,
					 pj_uint16_t client_id,
					 const pj_str_t &local_ip,
					 pj_uint16_t local_udp_port)
	: Noncopyable()
	, wrapper_(wrapper)
	, screens_(MAXIMAL_SCREEN_NUM)
	, av_index_map_(2)
	, avsproxy_id_(avsproxy_id)
	, width_(MININUM_SCREEN_WIDTH)
	, height_(NIMINUM_SCREEN_HEIGHT)
	, screen_mgr_res_(SCREEN_RES_3x3)
	, vertical_padding_(MININUM_PADDING)
	, horizontal_padding_(MININUM_PADDING)
	, client_id_(client_id)
	, local_tcp_sock_(-1)
	, local_udp_sock_(-1)
	, avsproxy_ip_(pj_str(avsproxy_ip.ptr))
	, avsproxy_tcp_port_(avsproxy_tcp_port)
	, local_ip_(local_ip)
	, local_udp_port_(local_udp_port)
	, caching_pool_()
	, pool_(NULL)
	, tcp_ev_(nullptr)
	, udp_ev_(nullptr)
	, evbase_(nullptr)
	, tcp_storage_offset_(0)
	, connector_thread_()
	, event_thread_()
	, active_(PJ_FALSE)
	, rooms_tree_ctl_()
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

	for (pj_uint32_t idx = 0; idx < MAXIMAL_SCREEN_NUM; ++ idx)
	{
		screens_[idx] = new Screen(idx, local_tcp_sock_, local_tcp_lock_);
	}
}

ScreenMgr::~ScreenMgr()
{
}

resolution_t ScreenMgr::GetDefaultResolution()
{
	return DEFAULT_RESOLUTION;
}

pj_status_t ScreenMgr::Prepare(const pj_str_t &log_file_name)
{
	pj_status_t status;

	pj_uint8_t retrys = 50;
	status = pj_open_tcp_clientport(&avsproxy_ip_, avsproxy_tcp_port_, local_tcp_sock_);
	RETURN_VAL_IF_FAIL( status == PJ_SUCCESS, status );

	do
	{
		status = pj_open_udp_transport(&local_ip_, local_udp_port_, local_udp_sock_);
	} while(status != PJ_SUCCESS && ((++ local_udp_port_), (-- retrys > 0)));
	RETURN_VAL_IF_FAIL( status == PJ_SUCCESS, status );

	pj_caching_pool_init(&caching_pool_, &pj_pool_factory_default_policy, 0);

	pool_ = pj_pool_create(&caching_pool_.factory, "AvsProxyClientPool", 1000, 1000, NULL);

	status = log_open(pool_, log_file_name);

	/* Init video format manager */
    status = pjmedia_video_format_mgr_create(pool_, 64, 0, NULL);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

	/* Init video converter manager */
    status = pjmedia_converter_mgr_create(pool_, NULL);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    /* Init event manager */
    status = pjmedia_event_mgr_create(pool_, 0, NULL);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

	/* Init video codec manager */
    status = pjmedia_vid_codec_mgr_create(pool_, NULL);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

	status = pjmedia_codec_ffmpeg_vid_init(NULL, &caching_pool_.factory);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

	evbase_ = event_base_new();
	RETURN_VAL_IF_FAIL( evbase_ != nullptr, -1 );

	udp_ev_ = event_new(evbase_, local_udp_sock_, EV_READ | EV_PERSIST, event_on_udp_read, this);
	RETURN_VAL_IF_FAIL( udp_ev_ != nullptr, -1 );

	tcp_ev_ = event_new(evbase_, local_tcp_sock_, EV_READ | EV_PERSIST, event_on_tcp_read, this);
	RETURN_VAL_IF_FAIL( tcp_ev_ != nullptr, -1 );

	int ret;
	ret = event_add(udp_ev_, NULL);
	RETURN_VAL_IF_FAIL( ret == 0, -1 );

	ret = event_add(tcp_ev_, NULL);
	RETURN_VAL_IF_FAIL( ret == 0, -1 );

	rooms_tree_ctl_.Prepare(wrapper_, IDC_ROOM_TREE_CTL_INDEX);

	for(pj_uint32_t idx = 0; idx < screens_.size(); ++ idx)
	{
		status = screens_[idx]->Prepare(pool_, CRect(0, 0, width_, height_), wrapper_, IDC_WALL_BASE_INDEX + idx);
	}

	return status;
}

pj_status_t ScreenMgr::Launch()
{
	(this->* screenmgr_func_array_[GET_FUNC_INDEX(screen_mgr_res_)])(width_, height_);

	active_ = PJ_TRUE;

	event_thread_ = thread(std::bind(&ScreenMgr::EventThread, this));
	//connector_thread_ = thread(std::bind(&ScreenMgr::ConnectorThread, this));
	sync_thread_pool_.Start();

	for(pj_uint32_t idx = 0; idx < screens_.size(); ++ idx)
	{
		screens_[idx]->Launch();
	}

	return PJ_SUCCESS;
}

void ScreenMgr::Destory()
{
	active_ = PJ_FALSE;
	event_base_loopexit(evbase_, NULL);
	sync_thread_pool_.Stop();

	pj_sock_close(local_tcp_sock_);
	pj_sock_close(local_udp_sock_);
}

void ScreenMgr::LinkScreenUser(Screen *screen, User *user)
{
	screen->LinkRoomUser(av_index_map_, user);
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
	rooms_tree_ctl_.MoveToRect(rect);

	screens_[0]->MoveToRect(CRect(MININUM_TREE_CTL_WIDTH, 0, MININUM_TREE_CTL_WIDTH + width, height));
}

void ScreenMgr::ChangeLayout_2x2(pj_uint32_t width, pj_uint32_t height)
{
	CRect rect(0, 0, MININUM_TREE_CTL_WIDTH, height * num_blocks_[1]);
	rooms_tree_ctl_.MoveToRect(rect);

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

			screens_[idx]->MoveToRect(rect);
		}
	}
}

void ScreenMgr::ChangeLayout_1x5(pj_uint32_t width, pj_uint32_t height)
{
	CRect rect(0, 0, MININUM_TREE_CTL_WIDTH, height * num_blocks_[2]);
	rooms_tree_ctl_.MoveToRect(rect);

	pj_uint32_t lstart = MININUM_TREE_CTL_WIDTH;
	unsigned idx = 0;

	pj_int32_t left = lstart, top = 0, right, bottom;
	right = left + width * 2 + horizontal_padding_;
	bottom = height * 2 + vertical_padding_;
	screens_[idx ++]->MoveToRect(CRect(left, top, right, bottom));

	left = right + horizontal_padding_;
	right = left + width;
	bottom = height;
	screens_[idx ++]->MoveToRect(CRect(left, top, right, bottom));

	top = bottom + vertical_padding_;
	bottom = top + height;
	screens_[idx ++]->MoveToRect(CRect(left, top, right, bottom));

	left = lstart;
	right = left + width;
	top = bottom + vertical_padding_;
	bottom = top + height;
	screens_[idx ++]->MoveToRect(CRect(left, top, right, bottom));

	left = right + horizontal_padding_;
	right = left + width;
	screens_[idx ++]->MoveToRect(CRect(left, top, right, bottom));

	left = right + horizontal_padding_;
	right = left + width;
	screens_[idx ++]->MoveToRect(CRect(left, top, right, bottom));
}

void ScreenMgr::ChangeLayout_3x3(pj_uint32_t width, pj_uint32_t height)
{
	CRect rect(0, 0, MININUM_TREE_CTL_WIDTH, height * num_blocks_[3]);
	rooms_tree_ctl_.MoveToRect(rect);

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

			screens_[idx]->MoveToRect(rect);
		}
	}
}

void ScreenMgr::HideAll()
{
	for(pj_uint8_t idx = 0; idx < screens_.size(); ++ idx)
	{
		screens_[idx]->HideWindow();
	}
}

void ScreenMgr::TcpParamScene(const pj_uint8_t *storage,
							  pj_uint16_t storage_len)
{
	RETURN_IF_FAIL(storage && (storage_len > 0));

	TcpParameter *param = NULL;
	TcpScene     *scene = NULL;
	pj_uint16_t type = (pj_uint16_t)ntohs(*(pj_uint16_t *)(storage + sizeof(param->length_)));

	switch(type)
	{
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
	}

	RETURN_IF_FAIL(param != nullptr && scene != nullptr);

	sync_thread_pool_.Schedule([=]()
	{
		scene->Maintain(param, &rooms_tree_ctl_);
		delete scene;
		delete param;
	});
}

void ScreenMgr::UdpParamScene(const pjmedia_rtp_hdr *rtp_hdr,
							  const pj_uint8_t *datagram,
							  pj_uint16_t datalen)
{
	enum { AUDIO_INDEX, VIDEO_INDEX };

	RETURN_IF_FAIL(rtp_hdr && datagram && datalen > 0);

	const pj_uint8_t media_index = (rtp_hdr->pt == RTP_MEDIA_VIDEO_TYPE) ? VIDEO_INDEX : 
		(rtp_hdr->pt == RTP_MEDIA_AUDIO_TYPE ? AUDIO_INDEX : -1);
	RETURN_IF_FAIL(media_index != -1);

	Screen *screen = nullptr;
	index_map_t::iterator pscreen_idx = av_index_map_[media_index].find(rtp_hdr->ssrc);
	if ( pscreen_idx != av_index_map_[media_index].end() )
	{
		index_map_t::mapped_type screen_idx = pscreen_idx->second;
		if(screen_idx >= 0 && screen_idx < screens_.size())
		{
			screen = screens_[screen_idx];
		}
	}

	RETURN_IF_FAIL(screen != nullptr);

	media_index == AUDIO_INDEX ?
		screen->AudioScene(datagram, datalen) :
		screen->VideoScene(datagram, datalen);
}

void ScreenMgr::EventOnTcpRead(evutil_socket_t fd, short event)
{
	pj_status_t status;
	RETURN_IF_FAIL(event & EV_READ);

	pj_ssize_t recvlen = MAX_STORAGE_SIZE - tcp_storage_offset_;
	status = pj_sock_recv(local_tcp_sock_,
		(char *)(tcp_storage_ + tcp_storage_offset_),
		&recvlen,
		0);

	if (recvlen > 0)
	{
		tcp_storage_offset_ += (pj_uint16_t)recvlen;
		pj_uint16_t packet_len = ntohs(*(pj_uint16_t *)tcp_storage_);
		pj_uint16_t total_len = packet_len + sizeof(packet_len);

		if (total_len > MAX_STORAGE_SIZE)
		{
			tcp_storage_offset_ = 0;
		}
		else if (total_len > tcp_storage_offset_)
		{
			return;
		}
		else if (total_len <= tcp_storage_offset_)
		{
			tcp_storage_offset_ -= total_len;

			TcpParamScene(tcp_storage_, total_len);

			if (tcp_storage_offset_ > 0)
			{
				memcpy(tcp_storage_, tcp_storage_ + total_len, tcp_storage_offset_);
			}
		}
	}
	else if (recvlen == 0)
	{
		pj_sock_close( local_tcp_sock_ );
		event_del( tcp_ev_ );
		connector_thread_ = thread(std::bind(&ScreenMgr::ConnectorThread, this));
	}
	else /* if ( recvlen < 0 ) */
	{
		pj_sock_close( local_tcp_sock_ );
		event_del( tcp_ev_ );
		connector_thread_ = thread(std::bind(&ScreenMgr::ConnectorThread, this));
	}
}

void ScreenMgr::EventOnUdpRead(evutil_socket_t fd, short event)
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
	status = pj_sock_recvfrom(local_udp_sock_, datagram, &datalen, 0, &addr, &addrlen);
	RETURN_IF_FAIL(status == PJ_SUCCESS);

	if (datalen >= sizeof(*rtp_hdr)
		&& datalen < (1 << 16))  // max data size is 2^16
	{
		status = pjmedia_rtp_decode_rtp(NULL,
			datagram, (int)datalen,
			&rtp_hdr, (const void **)&payload, &payload_len);
		RETURN_IF_FAIL(status == PJ_SUCCESS);

		UdpParamScene(rtp_hdr, datagram, (pj_uint16_t)datalen);
	}
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

void ScreenMgr::ConnectorThread()
{
	pj_thread_desc rtpdesc;
	pj_thread_t *thread = 0;
	
	if ( !pj_thread_is_registered() )
	{
		if ( pj_thread_register(NULL,rtpdesc,&thread) == PJ_SUCCESS )
		{
			unsigned sleep_msec = 5000;
			while(1)
			{
				if ( pj_open_tcp_clientport(&avsproxy_ip_, avsproxy_tcp_port_, local_tcp_sock_) == PJ_SUCCESS )
				{
					tcp_ev_ = event_new(evbase_, local_tcp_sock_, EV_READ | EV_PERSIST, event_on_tcp_read, this);
					if ( tcp_ev_ != nullptr && (event_add(tcp_ev_, NULL) == 0) )
					{
						LoginProxy();
						return;
					}
				}

				pj_thread_sleep( sleep_msec );
			}
		}
	}
}

pj_status_t ScreenMgr::LoginProxy()
{
	request_to_avs_proxy_login_t login;
	login.client_request_type = REQUEST_FROM_CLIENT_TO_AVSPROXY_LOGIN;
	login.proxy_id = avsproxy_id_;
	login.client_id = client_id_;
	pj_inet_aton(&local_ip_, &login.media_ip);
	login.media_port = local_udp_port_;
	login.Serialize();

	pj_ssize_t sndlen = sizeof(login);
	return SendTCPPacket(&login, &sndlen);
}

pj_status_t ScreenMgr::SendTCPPacket(const void *buf, pj_ssize_t *len)
{
	lock_guard<mutex> lock(local_tcp_lock_);
	return pj_sock_send(local_tcp_sock_, buf, len, 0);
}
