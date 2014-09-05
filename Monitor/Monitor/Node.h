#ifndef __AVS_PROXY_CLIENT_NODE__
#define __AVS_PROXY_CLIENT_NODE__

#include "pugixml.hpp"
#include "Config.h"
#include "Com.h"

enum __enum_node_tpye__
{
	TITLE,
	TITLE_NODE,
	TITLE_ROOM,
	TITLE_USER
};

class Node;
typedef map<pj_int32_t, Node *> node_map_t;
typedef set<Node *, order_cmp<Node>> node_set_t;
class Node
{
public:
	Node(pj_int32_t id, const pj_str_t &name, pj_uint32_t order, pj_uint32_t usercount, pj_uint8_t node_type);
	virtual ~Node() {}
	virtual void OnItemExpanded(CTreeCtrl &tree_ctrl, Node &parent) {}
	virtual void OnItemShrinked(CTreeCtrl &tree_ctrl, Node &parent) {}

protected:
	void AddNull(CTreeCtrl &tree_ctrl, HTREEITEM hParent);
	void AddNodeOrRoom(pj_int32_t id, Node *node, CTreeCtrl &tree_ctrl, HTREEITEM hParent);
	void DelNodeOrRoom(pj_int32_t id, CTreeCtrl &tree_ctrl);
	void DelAll(CTreeCtrl &tree_ctrl, Node &parent);
	virtual void ParseXML(const vector<pj_uint8_t> &xml) {}

public:
	HTREEITEM   tree_item_;
	pj_int32_t  id_;
	pj_str_t    name_;
	order_t     order_;
	pj_uint32_t usercount_;
	node_map_t  nodes_;
	node_set_t  nodes_order_;
	const pj_uint8_t node_type_;
};

#endif
