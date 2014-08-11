#ifndef __MONITOR_MESSAGE_QUEUE__
#define __MONITOR_MESSAGE_QUEUE__

#include <list>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "Com.h"

template <class T>
class MessageQueue
{
public:
	MessageQueue()
		: msg_wait_list_()
		, msg_proc_list_()
	{
	}

	void Wait()
	{
		std::unique_lock<std::mutex> wait_list_lock(wait_list_lock_);
		msg_queue_cv_.wait(wait_list_lock);

		if ( msg_wait_list_.size() > 0 )
		{
			msg_proc_list_.splice(msg_proc_list_.end(), msg_wait_list_);
		}
		wait_list_lock.unlock();
	}

	void Push(const T &msg)
	{
		std::unique_lock<std::mutex> wait_list_lock(wait_list_lock_);
		msg_wait_list_.push_back(msg);
		wait_list_lock.unlock();

		msg_queue_cv_.notify_one();
	}

	// Only one thread can access here, so we needn't take care of synchronization.
	T Pop()
	{
		T t;
		std::unique_lock<std::mutex> proc_list_lock(proc_list_lock_);
		if (msg_proc_list_.size() > 0)
		{
			std::list<T>::iterator it = msg_proc_list_.begin();
			t = *it;
			msg_proc_list_.erase(it);
			proc_list_lock.unlock();
			return t;
		}
		proc_list_lock.unlock();
		return t;
	}

	bool Empty()
	{
		proc_list_lock_.lock();
		bool result = (msg_proc_list_.size() == 0);
		proc_list_lock_.unlock();

		return result;
	}

	pj_uint32_t Size()
	{
		wait_list_lock_.lock();
		proc_list_lock_.lock();

		pj_uint32_t result = msg_wait_list_.size() + msg_proc_list_.size();

		proc_list_lock_.unlock();
		wait_list_lock_.unlock();

		return result;
	}

private:
	std::list<T> msg_wait_list_;
	std::list<T> msg_proc_list_;
	std::condition_variable msg_queue_cv_;
	std::mutex wait_list_lock_;
	std::mutex proc_list_lock_;
};

#endif
