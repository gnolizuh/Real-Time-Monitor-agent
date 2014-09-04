#include "stdafx.h"
#include "TitleRoom.h"

extern Config g_client_config;

TitleRoom::TitleRoom(pj_uint32_t id, const pj_str_t &name, order_t order, pj_uint32_t usercount)
	: Node(id, name, order, usercount)
{
}

TitleRoom::~TitleRoom()
{
}

void TitleRoom::OnItemExpanded(CTreeCtrl &tree_ctrl, Node &parent)
{
	// Clear all then insert all.
	DelAll(tree_ctrl, parent);

	//TO DO:

	if(users_.empty())
	{
		AddNull(tree_ctrl, tree_item_);
	}
}
