#include "stdafx.h"
#include "Node.h"

stack<Node *> g_traverse_stack;

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

void Node::Update(const pj_str_t &name, order_t order, pj_uint32_t usercount)
{
	if(name_.ptr != nullptr)
	{
		free(name_.ptr);
		name_.ptr = nullptr;
	}

	name_      = pj_str(strdup(name.ptr));
	order_     = order;
	usercount_ = usercount;
}

void Node::AddNull(CTreeCtrl &tree_ctrl)
{
	TVINSERTSTRUCT tvInsert;
	tvInsert.hParent = tree_item_;
	tvInsert.item.lParam = (LPARAM)0;
	tvInsert.item.pszText = _T("");
	tvInsert.item.mask = LVIF_TEXT | TVIF_PARAM;

	tree_ctrl.InsertItem(&tvInsert);
}

void Node::DelAll(CTreeCtrl &tree_ctrl)
{
	HTREEITEM node = tree_ctrl.GetChildItem(tree_item_);
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
		if(node != nullptr)
		{
			delete node;
			node = nullptr;
		}
	}

	nodes_.clear();
	nodes_order_.clear();
}

pj_bool_t Node::GetNodeOrRoom(pj_int32_t id, Node *&node)
{
	node_map_t::iterator pnode = nodes_.find(id);
	if(pnode != nodes_.end())
	{
		node = pnode->second;
		return PJ_TRUE;
	}
	else
	{
		node = nullptr;
		return PJ_FALSE;
	}
}

void Node::AddNodeOrRoom(pj_int32_t id,
						Node *node,
						CTreeCtrl &tree_ctrl)
{
	WCHAR gb_buf[128] = {0};
	UTF8_to_GB2312(gb_buf, sizeof(gb_buf), node->name_);

	nodes_.insert(node_map_t::value_type(id, node));

	TVINSERTSTRUCT tvInsert;
	tvInsert.hParent = tree_item_;
	tvInsert.hInsertAfter = tree_ctrl.GetNextItem(node->tree_item_, TVGN_NEXT);
	tvInsert.item.lParam = (LPARAM)node;
	tvInsert.item.pszText = gb_buf;
	tvInsert.item.mask = LVIF_TEXT | TVIF_PARAM;

	node->tree_item_ = tree_ctrl.InsertItem(&tvInsert);

	tvInsert.hParent = node->tree_item_;
	tvInsert.item.pszText = _T("");
	tvInsert.item.lParam = (LPARAM)0;
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

