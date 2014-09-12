#include "stdafx.h"
#include "TitleNode.h"

extern Config g_client_config;

TitleNode::TitleNode(pj_int32_t id, const pj_str_t &name, order_t order, pj_uint32_t usercount)
	: Node(id, name, order, usercount, TITLE_NODE)
{
}

TitleNode::~TitleNode()
{
}

void TitleNode::OnItemExpanded(CTreeCtrl &tree_ctrl, Node &parent)
{
	// Clear all then insert all.
	DelAll(tree_ctrl, parent);

	vector<pj_uint8_t> response;
	http_tls_get(g_client_config.tls_host, g_client_config.tls_port, g_client_config.tls_uri, id_, response);
	ParseXML(response, tree_ctrl, parent);

	if(nodes_.empty())
	{
		AddNull(tree_ctrl, parent.tree_item_);
	}
}

void TitleNode::ParseXML(const vector<pj_uint8_t> &xml, CTreeCtrl &tree_ctrl, Node &parent)
{
#define XML_STATIC_NODE  "static_node"
#define XML_DYNAMIC_NODE "dynamic_node"
#define XML_NODE         "node"
#define XML_ROOM_NODE    "room_node"
	pugi::xml_document doc;

	pugi::xml_parse_result result = doc.load_buffer(&xml[0], xml.size());
	RETURN_IF_FAIL(result);

	pugi::xml_node first_child = doc.root().first_child();
	if(strncmp((char *)first_child.name(), XML_STATIC_NODE, strlen(XML_STATIC_NODE)) == 0)
	{
		for(pugi::xml_node node = first_child.child(XML_NODE);
			node;
			node = node.next_sibling(XML_NODE))
		{
			node_map_t::key_type id = atoi(node.attribute("id").value());
			pj_str_t name = pj_str((char*)node.attribute("name").value());
			order_t order = atoi(node.attribute("order").value());
			pj_uint32_t usercount = atoi(node.attribute("usercount").value());

			TitleNode *title_node = new TitleNode(id, name, order, usercount);
			pj_assert(title_node);
			AddNodeOrRoom(id, title_node, tree_ctrl, parent.tree_item_);
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

			TitleRoom *title_room = new TitleRoom(&tree_ctrl, id, name, order, usercount);
			pj_assert(title_room);
			AddNodeOrRoom(id, title_room, tree_ctrl, parent.tree_item_);
		}
	}
}