#include "stdafx.h"
#include "RoomTreeCtl.h"

BEGIN_MESSAGE_MAP(RoomTreeCtl, CListBox)
END_MESSAGE_MAP()

RoomTreeCtl::RoomTreeCtl()
	: CTreeCtrl()
	, tree_item_(nullptr)
	, rooms_mutex_()
	, rooms_()
{
}

pj_status_t RoomTreeCtl::Prepare(const CWnd *wrapper, pj_uint32_t uid)
{
	BOOL result;
	result = Create(WS_VISIBLE | WS_TABSTOP | WS_CHILD | WS_BORDER
		| TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES
		| TVS_DISABLEDRAGDROP | TVS_NOTOOLTIPS,
		CRect(10, 10, 210, 410), (CWnd *)wrapper, uid);
	pj_assert(result == PJ_TRUE);

	TVINSERTSTRUCT tvInsert;
	tvInsert.hParent = NULL;
	tvInsert.hInsertAfter = NULL;
	tvInsert.item.mask = TVIF_TEXT;
	tvInsert.item.pszText = _T("·¿¼äºÅ");

	tree_item_ = InsertItem(&tvInsert);

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
		rooms_[room_id] = room;

		CString room_str;
		room_str.Format(_T("%d"), room_id);
		room->tree_item_ = InsertItem(room_str, tree_item_, TVI_LAST);
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
		room->users_[user_id] = user;

		CString user_str;
		user_str.Format(_T("%d"), user_id);
		user->tree_item_ = InsertItem(user_str, room->tree_item_, TVI_LAST);
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

void RoomTreeCtl::MoveToRect(const CRect &rect)
{
	this->MoveWindow(rect);
	this->ShowWindow(SW_SHOW);
}

void RoomTreeCtl::HideWindow()
{
	this->ShowWindow(SW_HIDE);
}