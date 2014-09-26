#include "stdafx.h"
#include "WatchsList.h"

WatchsList g_watchs_list;

WatchsList::WatchsList()
	: node_(nullptr)
	, title_(nullptr)
	, watching_(PJ_FALSE)
	, page_(0)
{
}

void WatchsList::Begin(Node *node, Title *title)
{
	End();

	node_ = node;
	title_ = title;
	watching_ = PJ_TRUE;

	node_->OnWatched(title);
}

void WatchsList::End()
{
	sinashow::SendMessage(WM_CLEAN_SCREENS, (WPARAM)0, (LPARAM)0);

	watching_ = PJ_FALSE;
	node_ = nullptr;
	title_ = nullptr;
	rooms_.clear();
	page_ = 0;

	while(!traverse_stack_.empty())
	{
		Pop();
	}
}

pj_status_t WatchsList::OnConfig(Node *node, CString &tips, pj_uint32_t &idc)
{
	RETURN_VAL_IF_FAIL(node != nullptr && (node->node_type_ == TITLE_NODE || node->node_type_ == TITLE_ROOM), PJ_EINVAL);

	idc = (watching_ && node == node_) ? IDC_MENU_UNLOOKUP : IDC_MENU_LOOKUP;
	tips.Format(L"%s%s",
		((watching_ && node == node_) ? L"取消查看" : L"查看"),
		node->node_type_ == TITLE_NODE ? L"大区" : L"房间");

	return PJ_SUCCESS;
}

Node *WatchsList::Top()
{
	return traverse_stack_.top();
}

void WatchsList::Push(Node *node)
{
	traverse_stack_.push(node);
}

void WatchsList::Pop()
{
	traverse_stack_.pop();
}

void WatchsList::OnTraverse()
{
	if(!traverse_stack_.empty())
	{
		Node *node = Top();
		Pop();
		if(node != nullptr)
		{
			node->OnWatched(title_);
		}
	}
	else
	{
		g_watchs_list.NextPage();
	}
}

void WatchsList::AddRoom(TitleRoom *room)
{
	RETURN_IF_FAIL(watching_ == PJ_TRUE);
	RETURN_IF_FAIL(room != nullptr);
	RETURN_IF_FAIL(title_ != nullptr && title_->BelowWatchedNode(room, node_));

	sinashow::SendMessage(WM_CONTINUE_TRAVERSE, (WPARAM)title_, (LPARAM)0);

	rooms_.insert(room);
}

pj_uint32_t WatchsList::Page()
{
	pj_uint32_t user_count = 0;
	room_set_t::iterator proom = rooms_.begin();
	for (; proom != rooms_.end(); ++proom)
	{
		room_set_t::value_type room = *proom;
		if (room != nullptr)
		{
			room->IncreaseCount(user_count);
		}
	}

	return (user_count % MAXIMAL_SCREEN_NUM == 0) ?
		(user_count / MAXIMAL_SCREEN_NUM) :
		(user_count / MAXIMAL_SCREEN_NUM) + 1;
}

void WatchsList::NextPage()
{
	RETURN_IF_FAIL(watching_ == PJ_TRUE);

	page_ = MIN(Page(), page_ + 1);
	OnShowPage();
}

void WatchsList::PrevPage()
{
	RETURN_IF_FAIL(watching_ == PJ_TRUE);

	page_ = MAX(1, page_ - 1);
	OnShowPage();
}

void WatchsList::OnShowPage()
{
	sinashow::SendMessage(WM_CLEAN_SCREENS, (WPARAM)0, (LPARAM)0);

	pj_uint32_t first = (page_ - 1) * MAXIMAL_SCREEN_NUM;

	pj_uint32_t offset = 0;
	room_set_t::iterator proom = rooms_.begin();
	for (; proom != rooms_.end(); ++proom)
	{
		room_set_t::value_type room = *proom;
		if (room != nullptr)
		{
			if (room->OnShowPage(offset, first) != PJ_SUCCESS)
			{
				return;
			}
		}
	}
}