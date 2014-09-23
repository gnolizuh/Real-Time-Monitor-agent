#ifndef __AVS_PROXY_CLIENT_WATCHS_LIST__
#define __AVS_PROXY_CLIENT_WATCHS_LIST__

#include <list>
#include <set>

#include "TitleRoom.h"
#include "Screen.h"
#include "Com.h"

using std::list;
using std::set;

typedef list<User *> watched_users_list_t;
typedef set<User *>  ordered_users_set_t;
class WatchsList
{
public:
	WatchsList();

	void Begin();
	void End();
	void OnAddUser(User *user);
	void OnDelUser(User *user);
	void OnModUser(User *user, pj_uint32_t audio_ssrc, pj_uint32_t video_ssrc);

private:
	pj_bool_t            watching_;
	watched_users_list_t bewatched_users_list_;
	ordered_users_set_t  ordered_users_set_;
};

extern WatchsList g_watchs_list;

#endif
