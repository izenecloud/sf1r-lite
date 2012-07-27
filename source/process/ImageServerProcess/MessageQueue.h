#ifndef __MESSAGEQUEUE__H__
#define __MESSAGEQUEUE__H__

#include <stdexcept>
#include <queue>
#include <time.h>
#include <errno.h>
#include "ScopeLock.h"
template<class T>
class MessageQueue
{
	public:
		typedef T MSG_TYPE;
		MessageQueue(std::size_t max_size_ = 1000) : max_size(max_size_) , close_flag(false)
		{
			if (max_size < 1)
			{
				throw std::runtime_error("MessageQueue::MessageQueue: max_size < 1");
			}
			pthread_mutex_init(&mutex, NULL);
			pthread_cond_init(&full_cond, NULL);
			pthread_cond_init(&empty_cond, NULL);
		}

		~MessageQueue()
		{
			close();       
			pthread_mutex_destroy(&mutex);
			pthread_cond_destroy(&full_cond);
			pthread_cond_destroy(&empty_cond);
		}

		void push(MSG_TYPE msg)
		{   
			ScopeLock lock(&mutex);
			while (msgs.size() >= max_size && !close_flag)
			{           
				pthread_cond_wait(&full_cond, &mutex);
			}
			if (close_flag)
			{
                return;
				//throw std::runtime_error("MessageQueue::push : msg queue has been shutdown.");
			}
			msgs.push(msg);
			if(msgs.size() == 1)
			{
				pthread_cond_broadcast(&empty_cond);
			}       
		}

		bool pushNonBlocking(MSG_TYPE msg)
		{
			ScopeLock lock(&mutex);
			if (msgs.size() >= max_size || close_flag)
			{
				return false;
			}
			msgs.push(msg);
			if(msgs.size() == 1)
			{
				pthread_cond_broadcast(&empty_cond);
			}
			return true;
		}

		void pop(MSG_TYPE &msg ) {
			ScopeLock lock(&mutex);
			while (msgs.empty() && !close_flag)
			{           
				pthread_cond_wait(&empty_cond, &mutex);
			}
			if (close_flag)
			{
				throw std::runtime_error("MessageQueue::pop : msg queue has been shutdown.");
			}
			msg = msgs.front();
			msgs.pop();
			if(msgs.size() == max_size - 1)
			{
				pthread_cond_broadcast(&full_cond);
			}       
		}

		bool popNonBlocking(MSG_TYPE &msg)
		{
			ScopeLock lock(&mutex);
			if (msgs.empty() || close_flag)
			{           
				return false;
			}       
			msg = msgs.front();
			msgs.pop();
			if(msgs.size() == max_size - 1)
			{
				pthread_cond_broadcast(&full_cond);
			}       
			return true;
		}

		void close()
		{
			close_flag = true;
			{
				ScopeLock lock(&mutex);
				fprintf(stderr, "MessageQueue close\n");
				pthread_cond_broadcast(&full_cond);
				pthread_cond_broadcast(&empty_cond);
			}       
		}

		std::size_t getMaxSize()
		{
			return max_size;
		}

		std::size_t getSize() 
		{
			ScopeLock lock(&mutex);
			//fprintf(stderr, "getSize()=%zu\n", msgs.size());
			return msgs.size();
		}

	private:
		pthread_cond_t full_cond;
		pthread_cond_t empty_cond;
		pthread_mutex_t mutex;
		std::queue<MSG_TYPE> msgs;   
		std::size_t const max_size;
		bool close_flag;
};

#endif
