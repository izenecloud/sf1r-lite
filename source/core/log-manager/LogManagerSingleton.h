#ifndef _LOG_MANAGER_SINGLETON_HPP
#define _LOG_MANAGER_SINGLETON_HPP

#include <boost/utility.hpp>
#include <boost/thread/once.hpp>
#include <boost/scoped_ptr.hpp>

// Warning: If T's constructor throws, instance() will return a null reference.

namespace sf1r {

template<class T>

class LogManagerSingleton : private boost::noncopyable
{

public:
    static T& instance()
    {
        boost::call_once(init, flag);
        return *t;
    }

    static void init() // never throws
    {
        t.reset(new T());
    }

protected:
    ~LogManagerSingleton() {}
     LogManagerSingleton() {}

private:
     static boost::scoped_ptr<T> t;
     static boost::once_flag flag;

};

}

template<class T> boost::scoped_ptr<T> sf1r::LogManagerSingleton<T>::t(0);
template<class T> boost::once_flag sf1r::LogManagerSingleton<T>::flag = BOOST_ONCE_INIT;

#endif
