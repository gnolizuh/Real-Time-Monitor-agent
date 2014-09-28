
// MonitorDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Monitor.h"
#include "pugixml.hpp"
#include "MonitorDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef __ABS_FILE__
#undef __ABS_FILE__
#endif

#define __ABS_FILE__ "Monitor.cpp"

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// CMonitorDlg 对话框

static pj_status_t init_param()
{
	pugi::xml_document doc;

    pugi::xml_parse_result result = doc.load_file("client.xml");
	RETURN_VAL_IF_FAIL(result, PJ_EINVAL);

	pugi::xml_node client = doc.child("client");
	g_client_config.client_id = atoi(client.attribute("id").value());
	g_client_config.local_ip = pj_str(strdup((char *)client.attribute("ip").value()));
	g_client_config.local_media_port = atoi(client.attribute("media_port").value());
	g_client_config.log_file_name = pj_str(strdup((char *)client.attribute("log_file_name").value()));
	g_client_config.tls_host = pj_str(strdup((char *)client.attribute("tls_host").value()));
	g_client_config.tls_port = atoi(client.attribute("tls_port").value());
	g_client_config.tls_uri = pj_str(strdup((char *)client.attribute("tls_uri").value()));
	g_client_config.rrtvms_fcgi_host = pj_str(strdup((char *)client.attribute("rrtvms_fcgi_host").value()));
	g_client_config.rrtvms_fcgi_port = atoi(client.attribute("rrtvms_fcgi_port").value());
	g_client_config.rrtvms_fcgi_uri = pj_str(strdup((char *)client.attribute("rrtvms_fcgi_uri").value()));

	return PJ_SUCCESS;
}

CMonitorDlg::CMonitorDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMonitorDlg::IDD, pParent)
	, is_draging_(PJ_FALSE)
	, draging_user_(nullptr)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMonitorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMonitorDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZING()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

void GetDesktopResolution(int& horizontal, int& vertical)
{
   RECT desktop;
   // Get a handle to the desktop window
   const HWND hDesktop = GetDesktopWindow();
   // Get the size of screen to the variable desktop
   GetWindowRect(hDesktop, &desktop);
   // The top left corner will have coordinates (0,0)
   // and the bottom right corner will have coordinates
   // (horizontal, vertical)
   horizontal = desktop.right;
   vertical = desktop.bottom;
}

void CMonitorDlg::event_func_proxy(evutil_socket_t fd, short event, void *arg)
{
	ev_function_t *pfunction = reinterpret_cast<ev_function_t *>(arg);
	(*pfunction)(fd, event, arg);
}

// CMonitorDlg 消息处理程序
static ScreenMgr *g_screen_mgr = NULL;
BOOL CMonitorDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	CRect rect;
	GetClientRect(&rect);

	SDL_Init( SDL_INIT_VIDEO );
	av_register_all();
	pj_init();
	
	PJ_LOG(5, (__ABS_FILE__, "Monitor start running....."));

	pj_status_t status;
	status = init_param();
	if(status != PJ_SUCCESS)
	{
		::AfxMessageBox(L"读取配置文件client.xml失败", MB_YESNO);
		exit(0);
	}

	g_screen_mgr = new ScreenMgr(this, 10, g_client_config.local_ip, g_client_config.local_media_port);
	RETURN_VAL_IF_FAIL(g_screen_mgr != nullptr, TRUE);

	status = g_screen_mgr->Prepare();
	if(status != PJ_SUCCESS)
	{
		::AfxMessageBox(L"系统准备失败", MB_YESNO);
		exit(0);
	}

	status = Prepare();
	if(status != PJ_SUCCESS)
	{
		::AfxMessageBox(L"系统准备失败", MB_YESNO);
		exit(0);
	}

	PJ_LOG(5, (__ABS_FILE__, "Monitor prepare ok!"));

	status = g_screen_mgr->Launch();
	if(status != PJ_SUCCESS)
	{
		::AfxMessageBox(L"系统启动失败", MB_YESNO);
		exit(0);
	}

	status = Launch();
	if(status != PJ_SUCCESS)
	{
		::AfxMessageBox(L"系统启动失败", MB_YESNO);
		exit(0);
	}

	PJ_LOG(5, (__ABS_FILE__, "Monitor launch ok!"));

	g_hwndTrackingTT = CreateTrackingToolTip(IDOK, m_hWnd, _T("ToolTip")); 

	ShowWindow(SW_MAXIMIZE);

	GetDlgItem(IDC_LAYOUT_BUTTON)->ShowWindow(SW_HIDE);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

pj_status_t CMonitorDlg::Prepare()
{
	int ret;
	ret = evutil_socketpair(AF_INET, SOCK_STREAM, 0, g_mainframe_pipe);

	event_base_ = event_base_new();
	RETURN_VAL_IF_FAIL(event_base_ != nullptr, PJ_EINVAL);

	ev_function_t function;
	ev_function_t *pfunction = nullptr;

	function = std::bind(&CMonitorDlg::EventOnPipe, this, std::placeholders::_1, std::placeholders::_2, nullptr);
	pfunction = new ev_function_t(function);
	event_ = event_new(event_base_, g_mainframe_pipe[0], EV_READ | EV_PERSIST, event_func_proxy, pfunction);
	RETURN_VAL_IF_FAIL(event_ != nullptr, PJ_EINVAL);

	ret = event_add(event_, NULL);
	RETURN_VAL_IF_FAIL(ret == 0, PJ_EINVAL);

	return PJ_SUCCESS;
}

pj_status_t CMonitorDlg::Launch()
{
	event_thread_ = thread(std::bind(&CMonitorDlg::EventThread, this));

	return PJ_SUCCESS;
}

BOOL CMonitorDlg::PreTranslateMessage(MSG *pMsg)
{
	if(WM_KEYFIRST <= pMsg->message && pMsg->message <= WM_KEYLAST)
	{
		if(pMsg->message == WM_KEYUP)
		{
			if(pMsg->wParam == VK_LEFT)
			{
				g_watchs_list.PrevPage();
			}
			else if(pMsg->wParam == VK_RIGHT)
			{
				g_watchs_list.NextPage();
			}
			else if(pMsg->wParam == VK_RETURN)
			{
				sinashow::SendMessage(WM_CHANGE_LAYOUT, (WPARAM)0, (LPARAM)0);
			}
		}
		return TRUE;
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}

void CMonitorDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}

	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMonitorDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CMonitorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMonitorDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	pj_assert(lpMMI != NULL);
	lpMMI->ptMinTrackSize.x = g_screen_mgr->GetDefaultResolution().x;
	lpMMI->ptMinTrackSize.y = g_screen_mgr->GetDefaultResolution().y;

	CDialog::OnGetMinMaxInfo(lpMMI);
}

void CMonitorDlg::OnSizing(UINT nSide, LPRECT lpRect)
{
	g_screen_mgr->GetSuitedSize(lpRect);

	CDialogEx::OnSizing(nSide, lpRect);
}

void CMonitorDlg::OnSize(UINT nType, int cx, int cy)
{
	RETURN_WITH_STATEMENT_IF_FAIL(g_screen_mgr != nullptr, CDialogEx::OnSize(nType, cx, cy));

	g_screen_mgr->Adjest( cx, cy );

	CDialogEx::OnSize(nType, cx, cy);
}

void CMonitorDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	is_draging_ = PJ_FALSE;
	draging_user_ = nullptr;
}

LRESULT CMonitorDlg::OnChangeLayout(WPARAM wParam, LPARAM lParam)
{
	struct resolution
	{
		enum_screen_mgr_resolution_t res;
	} ress[SCREEN_RES_END] = { SCREEN_RES_1x1, SCREEN_RES_2x2, SCREEN_RES_1x5, SCREEN_RES_3x3, SCREEN_RES_3x5};

	static enum_screen_mgr_resolution_t g_res_type = SCREEN_RES_1x1;

	g_screen_mgr->ChangeLayout(ress[g_res_type].res);

	g_res_type = (enum_screen_mgr_resolution_t)(( g_res_type + 1 ) % SCREEN_RES_END);

	return TRUE;
}

LRESULT CMonitorDlg::OnSelectUser(WPARAM wParam, LPARAM lParam)
{
	User *user = reinterpret_cast<User *>(lParam);
	RETURN_VAL_IF_FAIL(user, true);

	is_draging_ = PJ_TRUE;
	draging_user_ = user;

	return true;
}

LRESULT CMonitorDlg::OnLinkScreenUser(WPARAM wParam, LPARAM lParam)
{
	pj_uint32_t screen_idx = (pj_uint32_t)lParam;
	RETURN_VAL_IF_FAIL((screen_idx != INVALID_SCREEN_INDEX && draging_user_ && is_draging_), true);

	g_screen_mgr->LinkScreenUser(screen_idx, draging_user_);

	is_draging_ = PJ_FALSE;
	draging_user_ = nullptr;

	return true;
}

LRESULT CMonitorDlg::OnWatchRoomUser(WPARAM wParam, LPARAM lParam)
{
	User *user = reinterpret_cast<User *>(wParam);
	pj_uint32_t screen_idx = (pj_uint32_t)lParam;
	RETURN_VAL_IF_FAIL(user && screen_idx != INVALID_SCREEN_INDEX, true);

	g_screen_mgr->LinkScreenUser(screen_idx, user);

	return true;
}

LRESULT CMonitorDlg::OnUnlinkScreenUser(WPARAM wParam, LPARAM lParam)
{
	User   *user   = reinterpret_cast<User *>(wParam);
	Screen *screen = reinterpret_cast<Screen *>(lParam);
	RETURN_VAL_IF_FAIL((user != nullptr && screen != nullptr), true);

	g_screen_mgr->UnlinkScreenUser(screen, user);

	return true;
}

LRESULT CMonitorDlg::OnLinkRoom(WPARAM wParam, LPARAM lParam)
{
	Title *title = reinterpret_cast<Title *>(wParam);
	TitleRoom *title_room = (TitleRoom *)lParam;
	RETURN_VAL_IF_FAIL(title_room, true);

	pj_status_t status = g_screen_mgr->OnLinkRoom(title_room, title);
	if(status != PJ_SUCCESS && title == nullptr)
	{
		::AfxMessageBox(L"获取房间列表失败");
	}

	return true;
}

LRESULT CMonitorDlg::OnUnlinkRoom(WPARAM wParam, LPARAM lParam)
{
	TitleRoom *title_room = (TitleRoom *)lParam;
	RETURN_VAL_IF_FAIL(title_room, true);

	pj_status_t status = g_screen_mgr->OnUnlinkRoom(title_room);

	return true;
}

LRESULT CMonitorDlg::OnDisconnectAllProxys(WPARAM wParam, LPARAM lParam)
{
	g_screen_mgr->DelAllProxys();

	return true;
}

LRESULT CMonitorDlg::OnCleanScreens(WPARAM wParam, LPARAM lParam)
{
	g_screen_mgr->CleanScreens();

	return true;
}


void CMonitorDlg::EventOnPipe(evutil_socket_t fd, short event, void *arg)
{
	RETURN_IF_FAIL(event & EV_READ);

	param_t param;
	pj_ssize_t recvlen = sizeof(param_t);
	pj_status_t status;
	status = pj_sock_recv(g_mainframe_pipe[0], &param, &recvlen, 0);

	pj_assert(recvlen == sizeof(param_t));

	switch(param.Msg)
	{
		case WM_SELECT_USER:
			OnSelectUser(param.wParam, param.lParam);
			break;
		case WM_WATCH_ROOM_USER:
			OnWatchRoomUser(param.wParam, param.lParam);
			break;
		case WM_LINK_ROOM_USER:
			OnLinkScreenUser(param.wParam, param.lParam);
			break;
		case WM_UNLINK_ROOM_USER:
			OnUnlinkScreenUser(param.wParam, param.lParam);
			break;
		case WM_EXPANDEDROOM:
			OnLinkRoom(param.wParam, param.lParam);
			break;
		case WM_SHRINKEDROOM:
			OnUnlinkRoom(param.wParam, param.lParam);
			break;
		case WM_DISCONNECT_ALL_PROXYS:
			OnDisconnectAllProxys(param.wParam, param.lParam);
			break;
		case WM_CLEAN_SCREENS:
			OnCleanScreens(param.wParam, param.lParam);
			break;
		case WM_CONTINUE_TRAVERSE:
			reinterpret_cast<Title *>(param.wParam)->OnContinueTraverse();
			break;
		case WM_CHANGE_LAYOUT:
			OnChangeLayout(param.wParam, param.lParam);
			break;
		default:
			break;
	}
}

void CMonitorDlg::EventThread()
{
	pj_thread_desc rtpdesc;
	pj_thread_t *thread = 0;
	
	if ( !pj_thread_is_registered() )
	{
		if ( pj_thread_register(NULL, rtpdesc, &thread) == PJ_SUCCESS )
		{
			while(PJ_TRUE)
			{
				event_base_loop(event_base_, EVLOOP_ONCE);
			}
		}
	}
}
