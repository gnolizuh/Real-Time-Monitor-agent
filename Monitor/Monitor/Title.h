#ifndef __AVS_PROXY_CLIENT_TITLE__
#define __AVS_PROXY_CLIENT_TITLE__

#include "WatchsList.h"
#include "TitleRoom.h"
#include "TitleNode.h"
#include "ToolTip.h"
#include "Node.h"
#include "Com.h"

class TitleRoom;
class Title
	: public CTreeCtrl
	, public Node
{
public:
	Title(pj_uint32_t id, const pj_str_t &name, pj_uint32_t order);
	pj_status_t  Prepare(const CWnd *wrapper, pj_uint32_t uid);
	pj_status_t  Launch();
	virtual void OnDestory();
	void         AddNode(pj_uint32_t id, const pj_str_t &name, pj_uint32_t order, pj_uint32_t usercount);
	void         Perform();
	void         MoveToRect(const CRect &rect);
	void         HideWindow();
	pj_bool_t    BelowWatchedNode(TitleRoom *room, Node *node);
	LRESULT      OnContinueTraverse();

protected:
	afx_msg BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnMouseLeave();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnItemExpanded(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnBeginDrag(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnRightButtonClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLookUpNode(UINT nID);
	afx_msg void OnUnlookUpNode(UINT nID);
	DECLARE_MESSAGE_MAP()

private:
	Node *selected_node_;
};

#endif
