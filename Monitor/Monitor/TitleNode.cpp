#include "stdafx.h"
#include "TitleNode.h"

#ifdef __ABS_FILE__
#undef __ABS_FILE__
#endif

#define __ABS_FILE__ "TitleNode.cpp"

TitleNode::TitleNode(pj_int32_t id, const pj_str_t &name, order_t order, pj_uint32_t usercount)
	: Node(id, name, order, usercount, TITLE_NODE)
{
}

TitleNode::~TitleNode()
{
}

void TitleNode::OnDestory()
{
	node_map_t::iterator pnode = nodes_.begin();
	for(; pnode != nodes_.end(); ++ pnode)
	{
		node_map_t::mapped_type node = pnode->second;
		if(node != nullptr)
		{
			node->OnDestory();
			delete node;
			node = nullptr;
		}
	}

	nodes_.clear();
	nodes_order_.clear();
}

void TitleNode::OnWatched(void *ctrl)
{
	OnItemExpanded(*reinterpret_cast<CTreeCtrl *>(ctrl));

	node_set_t::reverse_iterator pnode = nodes_order_.rbegin();
	for(; pnode != nodes_order_.rend(); ++ pnode)
	{
		node_set_t::value_type node = *pnode;
		if(node != nullptr)
		{
			g_watchs_list.Push(node);
		}
	}

	g_watchs_list.OnTraverse();
}

void TitleNode::OnItemExpanded(CTreeCtrl &tree_ctrl)
{
	if(nodes_.empty())
	{
		DelAll(tree_ctrl);
	}

	vector<pj_uint8_t> response;
	http_tls_get(g_client_config.tls_host, g_client_config.tls_port, g_client_config.tls_uri, id_, response);
	ParseXML(response, tree_ctrl);

	if(nodes_.empty())
	{
		AddNull(tree_ctrl);
	}
}

void TitleNode::ParseXML(const vector<pj_uint8_t> &xml, CTreeCtrl &tree_ctrl)
{
#define XML_STATIC_NODE  "static_node"
#define XML_DYNAMIC_NODE "dynamic_node"
#define XML_NODE         "node"
#define XML_ROOM_NODE    "room_node"
	pugi::xml_document doc;
	node_map_t::mapped_type node = nullptr;
	set<node_map_t::key_type> nodes_id;

	pugi::xml_parse_result result = doc.load_buffer(&xml[0], xml.size());
	RETURN_IF_FAIL(result);

	pugi::xml_node first_child = doc.root().first_child();
	if(strncmp((char *)first_child.name(), XML_STATIC_NODE, strlen(XML_STATIC_NODE)) == 0)
	{
		for(pugi::xml_node xnode = first_child.child(XML_NODE);
			xnode;
			xnode = xnode.next_sibling(XML_NODE))
		{
			node_map_t::key_type id        = atoi(xnode.attribute("id").value());
			pj_str_t             name      = pj_str((char*)xnode.attribute("name").value());
			order_t              order     = atoi(xnode.attribute("order").value());
			pj_uint32_t          usercount = atoi(xnode.attribute("usercount").value());

			nodes_id.insert(id);

			if(GetNodeOrRoom(id, node))
			{
				if(node != nullptr)
				{
					node->Update(name, order, usercount);
				}
			}
			else
			{
				TitleNode *title_node = new TitleNode(id, name, order, usercount);
				pj_assert(title_node);
				AddNodeOrRoom(id, title_node, tree_ctrl);
			}
		}
	}
	else if(strncmp((char *)first_child.name(), XML_DYNAMIC_NODE, strlen(XML_DYNAMIC_NODE)) == 0)
	{
		for(pugi::xml_node room = first_child.child(XML_ROOM_NODE);
			room;
			room = room.next_sibling(XML_ROOM_NODE))
		{
			node_map_t::key_type id = atoi(room.attribute("id").value());
			pj_str_t name = pj_str((char*)room.attribute("name").value());
			order_t order = atoi(room.attribute("order").value());
			pj_uint32_t usercount = atoi(room.attribute("usercount").value());

			nodes_id.insert(id);

			if(GetNodeOrRoom(id, node))
			{
				if(node != nullptr)
				{
					node->Update(name, order, usercount);
				}
			}
			else
			{
				TitleRoom *title_room = new TitleRoom(&tree_ctrl, id, name, order, usercount);
				pj_assert(title_room);
				AddNodeOrRoom(id, title_room, tree_ctrl);
			}
		}
	}

	KickoutRedundantNodes(nodes_id);
}

// 如果再次点开"某大区"/"某房间", 可能会有下线的房间或者大区.
void TitleNode::KickoutRedundantNodes(const set<node_map_t::key_type> &nodes_id)
{
	node_map_t::iterator pnode = nodes_.begin();
	for(; pnode != nodes_.end();)
	{
		node_map_t::key_type id = pnode->first;
		node_map_t::mapped_type node = pnode->second;
		if(nodes_id.find(id) == nodes_id.end())
		{
			pnode = nodes_.erase(pnode);
			if(node != nullptr)
			{
				node->OnDestory(); // 清除下线房间或者大区
				delete node;
				node = nullptr;
			}
		}
		else
		{
			++ pnode;
		}
	}
}
