#ifndef __AVS_PROXY_CLIENT_TITLES_CTL__
#define __AVS_PROXY_CLIENT_TITLES_CTL__

#include <vector>
#include <set>

#include "pugixml.hpp"
#include "Config.h"
#include "Title.h"
#include "Com.h"

using std::vector;
using std::map;
using std::set;

class Room;

typedef map<pj_uint32_t, Title *> title_map_t;
typedef set<Title *, order_cmp<Title>> title_set_t;
class TitlesCtl
	: public Noncopyable
	, public CTabCtrl
{
public:
	TitlesCtl();

	pj_status_t  Prepare(const CWnd *wrapper, pj_uint32_t uid);
	pj_status_t  Launch();
	virtual void OnDestory();
	void         ParseXML(const vector<pj_uint8_t> &xml, pj_uint32_t uid);
	void         Perform();
	Title       *AddTitle(pj_int32_t room_id);
	void         DelTitle(pj_int32_t room_id);
	Title       *GetTitle(pj_int32_t room_id);
	void         GetTreeCtrlRect(LPRECT lpRect);
	void         MoveToRect(const CRect &rect);
	void         HideWindow();

protected:
	afx_msg void OnSelChange(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()

private:
	pj_uint8_t  selected_index_;
	title_map_t titles_;              // 用于以node_id检索title
	title_set_t titles_order_;        // 用于顺序显示各个title
};

#endif