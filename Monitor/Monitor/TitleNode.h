#ifndef __AVS_PROXY_CLIENT_TITLE_NODE__
#define __AVS_PROXY_CLIENT_TITLE_NODE__

#include "pugixml.hpp"
#include "Config.h"
#include "TitleRoom.h"
#include "Node.h"
#include "Com.h"

class TitleNode
	: public Node
{
public:
	TitleNode(pj_int32_t id, const pj_str_t &name, pj_uint32_t order, pj_uint32_t usercount);
	virtual ~TitleNode();

	void KickoutRedundantNodes(const set<node_map_t::key_type> &nodes_id);
	virtual void Destory();
	virtual void OnItemExpanded(CTreeCtrl &tree_ctrl, Node &parent);

protected:
	virtual void AddNodeOrRoom(pj_int32_t id, Node *node, CTreeCtrl &tree_ctrl, HTREEITEM hParent);
	virtual void DelNodeOrRoom(pj_int32_t id, CTreeCtrl &tree_ctrl);
	virtual void ParseXML(const vector<pj_uint8_t> &xml, CTreeCtrl &tree_ctrl, Node &parent);
};

#endif
