
// MonitorDlg.h : 头文件
//

#pragma once

#include "TitleRoom.h"
#include "ScreenMgr.h"
#include "Screen.h"
#include "Config.h"
#include "Com.h"

// CMonitorDlg 对话框
class CMonitorDlg : public CDialogEx
{
// 构造
public:
	CMonitorDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_MONITOR_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnSizing(UINT nSide, LPRECT lpRect);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnChangeLayout();
	afx_msg LRESULT OnSelectUser(WPARAM wParam, LPARAM lParam);
	afx_msg void    OnDropUpUser(UINT nFlags, CPoint point);
	afx_msg LRESULT OnLinkRoomUser(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUnlinkRoomUser(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLinkRoom(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUnlinkRoom(WPARAM wParam, LPARAM lParam);
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

private:
	pj_bool_t is_draging_;
	User      *draging_user_;
};
