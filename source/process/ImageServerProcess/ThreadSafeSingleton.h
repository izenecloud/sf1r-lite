#ifndef __TFSINGLETON__H__
#define __TFSINGLETON__H__
#define DECLARE_SINGLETON(class_name)   protected:\
                                        class_name(){};\
                                        class_name(const class_name &);\
                                        class_name &operator=(const class_name&);

#include <pthread.h>
template<typename T>
class ThreadSafeSingleton
{
	public:
		class Assistance 
		{
			private:
				pthread_mutex_t _mutex;
			public:
				Assistance() {
					pthread_mutex_init(&_mutex, NULL);
				};
				~Assistance() {
					pthread_mutex_destroy(&_mutex);
				};
				void Lock() {
					pthread_mutex_lock(&_mutex);
				};
				void UnLock() {
					pthread_mutex_unlock(&_mutex);
				};
		};
		static T* instance() {
			if (!T::_instance) {
				T::_assistance.Lock();
				if (!T::_instance) {
					T::_instance = new T;
				}
				T::_assistance.UnLock();
			}
			return T::_instance;
		};
		static void release() {
			if (T::_instance) {
				T::_assistance.Lock();
				if (T::_instance) {
					delete T::_instance;
					T::_instance = NULL;
				}
				T::_assistance.UnLock();
			}
		}
		~ThreadSafeSingleton(){};
	protected:
		static Assistance _assistance;
		static T* _instance;

    DECLARE_SINGLETON(ThreadSafeSingleton);
};
template<typename T> T* ThreadSafeSingleton<T>::_instance = NULL;
template<typename T> typename ThreadSafeSingleton<T>::Assistance ThreadSafeSingleton<T>::_assistance ;
#endif
