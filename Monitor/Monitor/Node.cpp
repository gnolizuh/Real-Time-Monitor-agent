#include "stdafx.h"
#include "Node.h"

Node::Node(pj_int32_t id, const pj_str_t &name, pj_uint32_t order, pj_uint32_t usercount, pj_uint8_t node_type)
	: tree_item_(nullptr)
	, id_(id)
	, name_(pj_str(strdup(name.ptr)))
	, order_(order)
	, usercount_(usercount)
	, nodes_()
	, nodes_order_()
	, node_type_(node_type)
{
}

void Node::AddNull(CTreeCtrl &tree_ctrl, HTREEITEM hParent)
{
	TVINSERTSTRUCT tvInsert;
	tvInsert.hParent = hParent;
	tvInsert.item.pszText = _T("");
	tvInsert.item.mask = LVIF_TEXT | TVIF_PARAM;

	tree_ctrl.InsertItem(&tvInsert);
}

void Node::AddNodeOrRoom(pj_int32_t id,
						Node *node,
						CTreeCtrl &tree_ctrl,
						HTREEITEM hParent)
{
	WCHAR gb_buf[128] = {0};
	UTF8_to_GB2312(gb_buf, sizeof(gb_buf), node->name_);

	DelNodeOrRoom(id, tree_ctrl);  // Make sure it isn't exist!

	nodes_.insert(node_map_t::value_type(id, node));

	TVINSERTSTRUCT tvInsert;
	tvInsert.hParent = hParent;
	tvInsert.hInsertAfter = tree_ctrl.GetNextItem(node->tree_item_, TVGN_NEXT);
	tvInsert.item.lParam = (LPARAM)node;
	tvInsert.item.pszText = gb_buf;
	tvInsert.item.mask = LVIF_TEXT | TVIF_PARAM;

	node->tree_item_ = tree_ctrl.InsertItem(&tvInsert);

	tvInsert.hParent = node->tree_item_;
	tvInsert.item.pszText = _T("");
	tvInsert.item.mask = LVIF_TEXT | TVIF_PARAM;

	tree_ctrl.InsertItem(&tvInsert);

	nodes_order_.insert(node);
}

void Node::DelNodeOrRoom(pj_int32_t id, CTreeCtrl &tree_ctrl)
{
	node_map_t::iterator pnode = nodes_.find(id);
	RETURN_IF_FAIL(pnode != nodes_.end());

	node_map_t::mapped_type node = pnode->second;
	pj_assert(node);
	nodes_.erase(pnode);

	tree_ctrl.DeleteItem(node->tree_item_);

	delete node;
	node = nullptr;
}

void Node::DelAll(CTreeCtrl &tree_ctrl, Node &parent)
{
	HTREEITEM node = tree_ctrl.GetChildItem(parent.tree_item_);
	HTREEITEM next = node;
	while(next)
	{
		node = next;
		next = tree_ctrl.GetNextSiblingItem(next);
		tree_ctrl.DeleteItem(node);
	}

	for(node_map_t::iterator pnode = nodes_.begin();
		pnode != nodes_.end();
		++ pnode)
	{
		node_map_t::mapped_type node = pnode->second;
		pj_assert(node);

		delete node;
		node = nullptr;
	}

	nodes_.clear();
	nodes_order_.clear();
}
