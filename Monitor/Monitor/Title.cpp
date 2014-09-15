#include "stdafx.h"
#include "Title.h"

BEGIN_MESSAGE_MAP(Title, CTreeCtrl)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED, OnItemExpanded)
	ON_NOTIFY_REFLECT(TVN_BEGINDRAG,    OnTvnBeginDrag)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
    ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
END_MESSAGE_MAP()

Title::Title(pj_uint32_t id, const pj_str_t &name, order_t order)
	: CTreeCtrl()
	, Node(id, name, order, 0, TITLE)
{
}

void Title::PreSubclassWindow()
{
   CTreeCtrl::PreSubclassWindow();
   EnableToolTips(TRUE);
}

pj_status_t Title::Prepare(const CWnd *wrapper, pj_uint32_t uid)
{
	BOOL result;
	result = Create(WS_VISIBLE | WS_TABSTOP | WS_CHILD | WS_BORDER
		| TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES,
		CRect(0, 0, 0, 0), (CWnd *)wrapper, uid);
	RETURN_VAL_IF_FAIL(result, PJ_EINVAL);

	return PJ_SUCCESS;
}

pj_status_t Title::Launch()
{
	return PJ_SUCCESS;
}

void Title::Destory()
{
}

void Title::AddNode(pj_uint32_t id, const pj_str_t &name, pj_uint32_t order, pj_uint32_t usercount)
{
	WCHAR gb_buf[128] = {0};
	UTF8_to_GB2312(gb_buf, sizeof(gb_buf), name);

	DelNode(id);  // Make sure it isn't exist!

	node_map_t::mapped_type node = new TitleNode(id, name, order, usercount);
	pj_assert(node);
	nodes_.insert(node_map_t::value_type(id, node));

	TVINSERTSTRUCT tvInsert;
	tvInsert.hParent = TVI_ROOT;
	tvInsert.hInsertAfter = GetNextItem(node->tree_item_, TVGN_NEXT);
	tvInsert.item.lParam = (LPARAM)node;
	tvInsert.item.pszText = gb_buf;
	tvInsert.item.mask = LVIF_TEXT | TVIF_PARAM;

	node->tree_item_ = InsertItem(&tvInsert);

	tvInsert.hParent = node->tree_item_;
	tvInsert.item.pszText = _T("");
	tvInsert.item.mask = LVIF_TEXT | TVIF_PARAM;

	InsertItem(&tvInsert);

	nodes_order_.insert(node);
}

void Title::DelNode(pj_uint32_t id)
{
	node_map_t::iterator pnode = nodes_.find(id);
	RETURN_IF_FAIL(pnode != nodes_.end());

	node_map_t::mapped_type node = pnode->second;
	pj_assert(node);
	nodes_.erase(pnode);

	DeleteItem(node->tree_item_);

	delete node;
	node = nullptr;
}

void Title::MoveToRect(const CRect &rect)
{
	MoveWindow(rect);
	ShowWindow(SW_SHOW);
}

void Title::HideWindow()
{
	ShowWindow(SW_HIDE);
}

void Title::OnItemExpanded(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	HTREEITEM pTreeItem = reinterpret_cast<HTREEITEM>(pNMTreeView->itemNew.hItem);

	Node *node = reinterpret_cast<Node *>(GetItemData(pTreeItem));
	RETURN_IF_FAIL(node);

	enum { EXPAND = 2, SHRINK = 1 };

	switch(pNMTreeView->action)
	{
		case EXPAND:
			node->OnItemExpanded(*this, *node);
			break;
		case SHRINK:
			node->OnItemShrinked(*this, *node);
			break;
	}
}

void Title::OnTvnBeginDrag(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
 
	HTREEITEM pTreeItem = reinterpret_cast<HTREEITEM>(pNMTreeView->itemNew.hItem);

	if(!ItemHasChildren(pTreeItem))
	{
		User *user = reinterpret_cast<User *>(GetItemData(pTreeItem));
		RETURN_IF_FAIL(user != nullptr);

		::SendMessage(AfxGetMainWnd()->m_hWnd, WM_SELECT_USER, 0, (LPARAM)user);
	}
}

BOOL Title::OnToolTipText(UINT id, NMHDR *pNMHDR, LRESULT *pResult)
{
	TOOLTIPTEXTA *pTTTA = (TOOLTIPTEXTA *)pNMHDR;
	TOOLTIPTEXTW *pTTTW = (TOOLTIPTEXTW *)pNMHDR;

	const MSG *pMessage = GetCurrentMessage(); 
	RETURN_VAL_IF_FAIL(pMessage != nullptr, FALSE);

	CPoint pt = pMessage->pt;  
	ScreenToClient(&pt);

	UINT nFlags;
	HTREEITEM hitem = HitTest(pt, &nFlags);
	RETURN_VAL_IF_FAIL(hitem != nullptr, true);

	if(!ItemHasChildren(hitem))
	{
		return true;
	}

	Node *node = reinterpret_cast<Node *>(GetItemData(hitem));
	RETURN_VAL_IF_FAIL(node != nullptr, FALSE);

	CString strTipText;
	strTipText.Format(L"房间ID: %d 人数: %u", node->id_, node->usercount_);
	if (pNMHDR->code == TTN_NEEDTEXTA)
	{
		_wcstombsz(pTTTA->szText, strTipText, 80);
	}
	else
	{
		lstrcpyn(pTTTW->szText, strTipText, 80);
	}
 
	return TRUE;
}
