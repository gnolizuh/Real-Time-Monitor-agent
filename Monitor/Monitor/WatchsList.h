#ifndef __AVS_PROXY_CLIENT_WATCHS_LIST__
#define __AVS_PROXY_CLIENT_WATCHS_LIST__

#include <stack>
#include <vector>
#include <list>
#include <set>

#include "TitleRoom.h"
#include "Title.h"
#include "Screen.h"
#include "Com.h"

using std::stack;
using std::vector;
using std::list;
using std::set;

class Title;
class TitleRoom;
typedef list<User *> users_list_t;
typedef set<TitleRoom *, order_cmp<TitleRoom>> room_set_t;
class WatchsList
{
public:
	WatchsList();

	pj_status_t OnConfig(Node *node, CString &tips, pj_uint32_t &idc);
	void  Begin(Node *node, Title *title);
	void  End();
	void  AddRoom(TitleRoom *room);
	void  NextPage();
	void  PrevPage();
	Node *Top();
	void  Push(Node *node);
	void  Pop();
	void  OnTraverse();
	inline pj_bool_t Watching() const { return watching_; }

private:
	pj_uint32_t Page();
	void OnShowPage();

private:
	Node       *node_;
	Title      *title_;
	pj_bool_t   watching_;
	pj_uint32_t page_;
	room_set_t  rooms_;
	stack<Node *> traverse_stack_;
};

extern WatchsList g_watchs_list;

#endif
