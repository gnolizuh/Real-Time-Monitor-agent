#ifndef __AVS_PROXY_POOL_THREAD__
#define __AVS_PROXY_POOL_THREAD__

#include <vector>
#include <algorithm>

#include "MessageQueue.hpp"

// Min Heap
static struct min_heap_cmp_func_t
{
	template<class HeapElem>
	bool operator () (HeapElem e1, HeapElem e2)
	{
		return (e1->Load() > e2->Load());
	}
} min_heap_cmp_func;

template<class T>
class InternalThread
{
public:
	InternalThread(pj_uint32_t thread_index)
		: thread_index_(thread_index)
		, active_(PJ_FALSE)
		, msg_queue_()
		, internal_thread_()
	{
	}

	void Start()
	{
		active_ = PJ_TRUE;
		internal_thread_ = std::thread([=]
		{
			pj_thread_desc rtpdesc;
			pj_thread_t *thread = 0;

			if ( !pj_thread_is_registered() )
			{
				pj_thread_register(NULL, rtpdesc, &thread);
			}

			while (active_)
			{
				msg_queue_.Wait();

				do
				{
					T package(msg_queue_.Pop());

					package();
				} while ( !msg_queue_.Empty() );
			}
		});
	}

	void Stop()
	{
		msg_queue_.Push([this]
		{
			active_ = PJ_FALSE;
		});
	}

	void Push(const T &t)
	{
		msg_queue_.Push(t);
	}

	void Join()
	{
		internal_thread_.join();
	}

	inline pj_uint32_t Load()
	{
		return msg_queue_.Size();
	}

private:
	pj_uint32_t     thread_index_;
	pj_bool_t       active_;
	MessageQueue<T> msg_queue_;
	std::thread     internal_thread_;
};

template<class T>
class PoolThread
{
public:
	PoolThread(pj_uint32_t threads_count)
		: threads_count_(threads_count)
		, threads_(threads_count)
	{
		for(pj_uint32_t idx = 0; idx < threads_count; ++ idx)
		{
			threads_[idx] = new InternalThread<T>(idx);
		}

		std::make_heap(threads_.begin(), threads_.end(), min_heap_cmp_func);
	}

	void Start()
	{
		for(pj_uint32_t idx = 0; idx < threads_count_; ++ idx)
		{
			threads_[idx]->Start();
		}
	}
	void Stop()
	{
		for(pj_uint32_t idx = 0; idx < threads_count_; ++ idx)
		{
			threads_[idx]->Stop();
			threads_[idx]->Join();
		}
	}

	void Schedule(const T &t)
	{
		// First element indicate the lightest load thread.
		std::lock_guard<mutex> lock(threads_lock_);
		threads_[0]->Push(t);
		std::make_heap(threads_.begin(), threads_.end(), min_heap_cmp_func);
	}

private:
	std::mutex                       threads_lock_;
	pj_uint32_t                      threads_count_;
	std::vector<InternalThread<T> *> threads_;
};

#endif
