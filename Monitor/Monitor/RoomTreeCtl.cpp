#include "stdafx.h"
#include "RoomTreeCtl.h"

BEGIN_MESSAGE_MAP(RoomTreeCtl, CListBox)
	ON_NOTIFY_REFLECT(TVN_BEGINDRAG, OnTvnBeginDrag)
END_MESSAGE_MAP()

RoomTreeCtl::RoomTreeCtl()
	: CTreeCtrl()
	, root_item_(nullptr)
	, rooms_mutex_()
	, rooms_()
{
}

pj_status_t RoomTreeCtl::Prepare(const CWnd *wrapper, pj_uint32_t uid)
{
	BOOL result;
	result = Create(WS_VISIBLE | WS_TABSTOP | WS_CHILD | WS_BORDER
		| TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_NOTOOLTIPS,
		CRect(10, 10, 210, 410), (CWnd *)wrapper, uid);
	RETURN_VAL_IF_FAIL(result, PJ_EINVAL);

	TVINSERTSTRUCT tvInsert;
	tvInsert.hParent = root_item_;
	tvInsert.hInsertAfter = TVI_LAST;
	tvInsert.item.lParam = (LPARAM)this;
	tvInsert.item.pszText = _T("房间号");
	tvInsert.item.mask = LVIF_TEXT | TVIF_PARAM;

	root_item_ = InsertItem(_T("房间号"), NULL, NULL);
	RETURN_VAL_IF_FAIL(root_item_, PJ_EINVAL);

	return PJ_SUCCESS;
}

pj_status_t RoomTreeCtl::Launch()
{
	return PJ_SUCCESS;
}

void RoomTreeCtl::Destory()
{
}

Room *RoomTreeCtl::AddRoom(pj_int32_t room_id)
{
	lock_guard<mutex> lock(rooms_mutex_);
	rooms_map_t::iterator proom = rooms_.find(room_id);
	rooms_map_t::mapped_type room = nullptr;
	if(proom != rooms_.end())
	{
		room = proom->second;
	}
	else
	{
		room = new Room();
		room->room_id_ = room_id;
		rooms_[room_id] = room;

		wchar_t str_room_id[32];
		swprintf(str_room_id, sizeof(str_room_id) - 1, L"%u", room_id);

		TVINSERTSTRUCT tvInsert;
		tvInsert.hParent = root_item_;
		tvInsert.hInsertAfter = TVI_LAST;
		tvInsert.item.lParam = (LPARAM)room;
		tvInsert.item.pszText = str_room_id;
		tvInsert.item.mask = LVIF_TEXT | TVIF_PARAM;

		room->tree_item_ = InsertItem(&tvInsert);
	}

	return room;
}

void RoomTreeCtl::DelRoom(pj_int32_t room_id)
{
	lock_guard<mutex> lock(rooms_mutex_);
	rooms_map_t::iterator proom = rooms_.find(room_id);
	rooms_map_t::mapped_type room = nullptr;
	if(proom != rooms_.end())
	{
		rooms_.erase(proom);
		room = proom->second;
		DeleteItem(room->tree_item_);
	}

	if(room != nullptr)
	{
		delete room;
		room = nullptr;
	}
}

Room *RoomTreeCtl::GetRoom(pj_int32_t room_id)
{
	lock_guard<mutex> lock(rooms_mutex_);
	rooms_map_t::iterator proom = rooms_.find(room_id);
	rooms_map_t::mapped_type room = nullptr;
	if(proom != rooms_.end())
	{
		room = proom->second;
	}

	return room;
}

User *RoomTreeCtl::AddUser(Room *room, pj_int64_t user_id)
{
	lock_guard<mutex> lock(rooms_mutex_);
	users_map_t::iterator puser = room->users_.find(user_id);
	users_map_t::mapped_type user = nullptr;
	if(puser != room->users_.end())
	{
		user = puser->second;
	}
	else
	{
		user = new User();
		user->user_id_ = user_id;
		user->room_id_ = room->room_id_;
		room->users_[user_id] = user;

		wchar_t str_user_id[32];
		swprintf(str_user_id, sizeof(str_user_id) - 1, L"%u", user_id);

		TVINSERTSTRUCT tvInsert;
		tvInsert.hParent = room->tree_item_;
		tvInsert.hInsertAfter = TVI_LAST;
		tvInsert.item.lParam = (LPARAM)user;
		tvInsert.item.pszText = str_user_id;
		tvInsert.item.mask = LVIF_TEXT | TVIF_PARAM;

		user->tree_item_ = InsertItem(&tvInsert);
	}

	return user;
}

void RoomTreeCtl::DelUser(Room *room, pj_int64_t user_id)
{
	lock_guard<mutex> lock(rooms_mutex_);
	users_map_t::iterator puser = room->users_.find(user_id);
	users_map_t::mapped_type user = nullptr;
	if(puser != room->users_.end())
	{
		room->users_.erase(puser);
		user = puser->second;
		DeleteItem(user->tree_item_);
	}
	
	if(user != nullptr)
	{
		delete user;
		user = nullptr;
	}
}

User *RoomTreeCtl::GetUser(Room *room, pj_int64_t user_id)
{
	lock_guard<mutex> lock(rooms_mutex_);
	users_map_t::iterator puser = room->users_.find(user_id);
	users_map_t::mapped_type user = nullptr;
	if(puser != room->users_.end())
	{
		user = puser->second;
	}

	return user;
}

void RoomTreeCtl::ModMedia(User *user, pj_uint32_t audio_ssrc, pj_uint32_t video_ssrc)
{
	lock_guard<mutex> lock(rooms_mutex_);
	user->audio_ssrc_ = audio_ssrc;
	user->video_ssrc_ = video_ssrc;
}

void RoomTreeCtl::OnTvnBeginDrag(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	HTREEITEM pTreeItem = reinterpret_cast<HTREEITEM>(pNMTreeView->itemNew.hItem);

	if(!ItemHasChildren(pTreeItem))
	{
		CString str;
		str.Format(_T("The currently selected item is \"%s\"\n"),
			(LPCTSTR)GetItemText(pTreeItem));
		TRACE(str);

		User *user = reinterpret_cast<User *>(GetItemData(pTreeItem));
		if(user)
		{
			::SendMessage(AfxGetMainWnd()->m_hWnd, WM_BEGINDRAGITEM, 0, (LPARAM)user); // let Mainframe knowns.
			TRACE("user_id:%ld\n", user->user_id_);
		}
	}

	TRACE("TvnBeginDrag x:%ld y:%ld\n", pNMTreeView->ptDrag.x, pNMTreeView->ptDrag.y);
}

void RoomTreeCtl::MoveToRect(const CRect &rect)
{
	MoveWindow(rect);
	ShowWindow(SW_SHOW);
}

void RoomTreeCtl::HideWindow()
{
	ShowWindow(SW_HIDE);
}
