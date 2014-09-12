#include "stdafx.h"
#include "TitlesCtl.h"

BEGIN_MESSAGE_MAP(TitlesCtl, CTabCtrl)
	ON_NOTIFY_REFLECT(TCN_SELCHANGE, OnSelChange)
END_MESSAGE_MAP()

extern Config g_client_config;

TitlesCtl::TitlesCtl()
	: selected_index_(0)
	, titles_()
	, titles_order_()
{
}

pj_status_t TitlesCtl::Prepare(const CWnd *wrapper, pj_uint32_t uid)
{
	BOOL result;
	result = Create(TCS_TABS | TCS_FIXEDWIDTH | TCS_VERTICAL | 
		WS_BORDER | WS_CHILD | WS_VISIBLE,
		CRect(0, 0, 0, 0), (CWnd *)wrapper, uid);

	vector<pj_uint8_t> response;
	http_tls_get(g_client_config.tls_host, g_client_config.tls_port, g_client_config.tls_uri, 0, response);
	ParseXML(response, uid);

	Perform();
	
	return PJ_SUCCESS;
}

pj_status_t TitlesCtl::Launch()
{
	return PJ_SUCCESS;
}

void TitlesCtl::Destory()
{
}

void TitlesCtl::ParseXML(const vector<pj_uint8_t> &xml, pj_uint32_t uid)
{
#define XML_ROOT_NAME    "table_list_index"
#define XML_SERVICE_NAME "service"
#define XML_NODE_NAME    "node"
	pugi::xml_document doc;

	pugi::xml_parse_result result = doc.load_buffer(&xml[0], xml.size());
	RETURN_IF_FAIL(result);

	pugi::xml_node first_child = doc.root().first_child();
	pj_assert(strncmp((char *)first_child.name(), XML_ROOT_NAME, strlen(XML_ROOT_NAME)) == 0);
	for(pugi::xml_node service = first_child.child(XML_SERVICE_NAME);
		service;
		service = service.next_sibling(XML_SERVICE_NAME))
	{
		title_map_t::key_type id = atoi(service.attribute("id").value());
		pj_str_t name = pj_str((char*)service.attribute("name").value());
		order_t order = atoi(service.attribute("order").value());
		pj_uint32_t usercount;
		title_map_t::iterator pservice = titles_.find(id);
		pj_assert(pservice == titles_.end());

		Title *title = new Title(id, name, order);
		pj_assert(title);
		title->Prepare(this, ++ uid);
		title->ShowWindow(SW_HIDE);

		titles_.insert(title_map_t::value_type(id, title));
		titles_order_.insert(title);
		for(pugi::xml_node node = service.child(XML_NODE_NAME);
			node;
			node = node.next_sibling(XML_NODE_NAME))
		{
			id = atoi(node.attribute("id").value());
			name = pj_str((char*)node.attribute("name").value());
			order = atoi(node.attribute("order").value());
			usercount = atoi(node.attribute("usercount").value());
			title->AddNode(id, name, order, usercount);
		}
	}
}

void TitlesCtl::Perform()
{
	TCITEM tcItem;
	tcItem.mask = TCIF_TEXT;

	int i = 0;
	for(title_set_t::iterator ptitle = titles_order_.begin();
		ptitle != titles_order_.end();
		++ ptitle, ++ i)
	{
		title_set_t::key_type title = *ptitle;

		WCHAR gb_buf[128] = {0};
		UTF8_to_GB2312(gb_buf, sizeof(gb_buf), title->name_);

		tcItem.pszText = gb_buf;
		InsertItem(i, &tcItem);

		if(i == selected_index_)
		{
			title->ShowWindow(SW_SHOW);
		}
	}
}

void TitlesCtl::GetTreeCtrlRect(LPRECT lpRect)
{
	RETURN_IF_FAIL(lpRect);

	RECT crect;
	GetClientRect(&crect);

	RECT irect;
	GetItemRect(0, &irect);

	lpRect->left   = irect.right - irect.left;
	lpRect->right  = crect.left;
	lpRect->top    = crect.top;
	lpRect->bottom = crect.bottom;
}

void TitlesCtl::MoveToRect(const CRect &rect)
{
	RECT irect;
	GetItemRect(0, &irect);

	CRect title_rect(irect.right, rect.top, rect.right, rect.bottom);
	for(title_set_t::iterator ptitle = titles_order_.begin();
		ptitle != titles_order_.end();
		++ ptitle)
	{
		title_set_t::key_type title = *ptitle;
		title->MoveWindow(title_rect);
	}

	MoveWindow(rect);
	ShowWindow(SW_SHOW);
}

void TitlesCtl::HideWindow()
{
	ShowWindow(SW_HIDE);
}

void TitlesCtl::OnSelChange(NMHDR* pNMHDR, LRESULT* pResult)
{
	selected_index_ = GetCurSel();

	int i = 0;
	for(title_set_t::iterator ptitle = titles_order_.begin();
		ptitle != titles_order_.end();
		++ ptitle, ++ i)
	{
		title_set_t::key_type title = *ptitle;
		title->ShowWindow(i == selected_index_ ? SW_SHOW : SW_HIDE);
	}
}
