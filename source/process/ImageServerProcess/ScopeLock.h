#ifndef __SCOPELOCK__H__
#define __SCOPELOCK__H__
#include <pthread.h>
#include <stdlib.h>
#include "stdio.h"
#define LOCK_MUTEX(mutex)   do {if ((pthread_mutex_lock(mutex)) != 0) {\
	                                fprintf(stderr, "failed to lock mutex \n");\
	                                exit(-1);}}while(0)

#define UNLOCK_MUTEX(mutex) do {if ((pthread_mutex_unlock(mutex)) != 0) {\
	                                fprintf(stderr, "failed to unlock mutex \n");\
	                                exit(-1);}}while(0)

class ScopeLock
{
	public:
		ScopeLock(pthread_mutex_t *mutex)
		{
			this->mutex = mutex;
			LOCK_MUTEX(this->mutex);
		}
		~ScopeLock() 
		{
			UNLOCK_MUTEX(this->mutex);
		}
	private:
		pthread_mutex_t *mutex;
};

#endif
