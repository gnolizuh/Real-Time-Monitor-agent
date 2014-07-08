#ifndef __MONITOR_MESSAGE_QUEUE__
#define __MONITOR_MESSAGE_QUEUE__

#include <list>
#include <SDL_thread.h>

using std::list;

namespace sinashow
{

template <class T>
class MessageQueue
{
public:
	MessageQueue()
	{
		msg_queue_sem_ = SDL_CreateSemaphore(0);
		msg_queue_lock_ = SDL_CreateMutex();
	}

	~MessageQueue()
	{
		SDL_DestroySemaphore(msg_queue_sem_);
		SDL_DestroyMutex(msg_queue_lock_);
	}

	void Wait()
	{
		SDL_SemWait(msg_queue_sem_);

		SDL_LockMutex(msg_queue_lock_);
		if ( msg_wait_list_.size() > 0 )
		{
			msg_proc_list_.merge(msg_wait_list_);
		}
		SDL_UnlockMutex(msg_queue_lock_);
	}

	void Push(const T msg)
	{
		SDL_LockMutex(msg_queue_lock_);
		msg_wait_list_.push_back(msg);
		SDL_UnlockMutex(msg_queue_lock_);

		SDL_SemPost(msg_queue_sem_);
	}

	T Pop()
	{
		if (msg_proc_list_.size() > 0)
		{
			list<T>::iterator it = msg_proc_list_.begin();
			T t = *it;
			msg_proc_list_.erase(it);
			return t;
		}
		return NULL;
	}

private:
	list<T> msg_wait_list_;
	list<T> msg_proc_list_;
	SDL_semaphore *msg_queue_sem_;
	SDL_mutex *msg_queue_lock_;
};

};

#endif
