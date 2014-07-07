
// MonitorDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Monitor.h"
#include "MonitorDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


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



CMonitorDlg::CMonitorDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMonitorDlg::IDD, pParent)
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
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_BN_CLICKED(IDC_BUTTON1, &CMonitorDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// CMonitorDlg 消息处理程序

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

	pj_int32_t width = abs(rect.bottom - rect.top);
	pj_int32_t height = abs(rect.right - rect.left);

	SDL_Init( SDL_INIT_VIDEO );

	ScreenMgr::GetInstance()->Prepare(this);
	ScreenMgr::GetInstance()->Launch();
	ScreenMgr::GetInstance()->Adjest(width, height);

	this->MoveWindow(CRect(0, 0, width, height));
	this->ShowWindow(SW_SHOW);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
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
	lpMMI->ptMinTrackSize.x = ScreenMgr::GetInstance()->GetDefaultResolution().x;
	lpMMI->ptMinTrackSize.y = ScreenMgr::GetInstance()->GetDefaultResolution().y;

	CDialog::OnGetMinMaxInfo(lpMMI);
}

// cx, cy means client area's width and height.
void CMonitorDlg::OnSize(UINT nType, int cx, int cy)
{
	switch(nType)
	{
		case SIZE_MAXIMIZED:
			break;
		case SIZE_MINIMIZED:
			break;
		case SIZE_RESTORED:
			break;
		default:
			break;
	}

	ScreenMgr::GetInstance()->Adjest( cx, cy );

	CDialog::OnSize(nType, cx, cy);
}

void CMonitorDlg::OnBnClickedButton1()
{
	struct resolution
	{
		pj_uint32_t width;
		pj_uint32_t height;
		screen_mgr_res_t res;
	} ress[4] = 
	{
		{400, 400, SCREEN_RES_1x1},
		{200, 200, SCREEN_RES_2x2},
		{100, 100, SCREEN_RES_1x5},
		{100, 100, SCREEN_RES_3x3},
	};

	static screen_mgr_res_t g_res_type = SCREEN_RES_1x1;

	ScreenMgr::GetInstance()->Flex(ress[g_res_type].width,
		ress[g_res_type].height,
		ress[g_res_type].res);

	g_res_type = (screen_mgr_res_t)(( g_res_type + 1 ) % 4);
}
