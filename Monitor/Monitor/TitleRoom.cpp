#include "stdafx.h"
#include "TitleRoom.h"

extern Config g_client_config;
extern Pipe   g_client_pipe;

TitleRoom::TitleRoom(pj_int32_t id, const pj_str_t &name, order_t order, pj_uint32_t usercount)
	: Node(id, name, order, usercount, TITLE_ROOM)
{
}

TitleRoom::~TitleRoom()
{
}

void TitleRoom::OnItemExpanded(CTreeCtrl &tree_ctrl, Node &parent)
{
	// Clear all then insert all.
	DelAll(tree_ctrl, parent);

	//Let the mainframe knowns.
	::SendMessage(AfxGetMainWnd()->m_hWnd, WM_EXPANDEDROOM, 0, (LPARAM)this);

	vector<pj_uint8_t> buffer;
	pj_status_t status = g_client_pipe.Read(buffer);

	if(users_.empty())
	{
		AddNull(tree_ctrl, tree_item_);
	}
}
