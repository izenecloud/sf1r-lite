/**
 * @file ResourceManager.h
 * @brief This class is used to manage the resource instance.
 *        It supports get/set the resource instance by multi-threads.
 */

#ifndef SF1R_RESOURCE_MANAGER_H
#define SF1R_RESOURCE_MANAGER_H

#include <la-manager/KNlpWrapper.h>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

namespace sf1r
{

template <class Resource>
class ResourceManager
{
public:
    static boost::shared_ptr<Resource> getResource()
    {
        ScopedReadLock lock(mutex_);
        return resource_;
    }

    static void setResource(boost::shared_ptr<Resource> newResource)
    {
        ScopedWriteLock lock(mutex_);
        resource_ = newResource;
    }

private:
    typedef boost::shared_mutex MutexType;
    typedef boost::shared_lock<MutexType> ScopedReadLock;
    typedef boost::unique_lock<MutexType> ScopedWriteLock;

    static MutexType mutex_;
    static boost::shared_ptr<Resource> resource_;
};

template <class Resource>
boost::shared_mutex ResourceManager<Resource>::mutex_;

template <class Resource>
boost::shared_ptr<Resource> ResourceManager<Resource>::resource_;

/** instantiation desclaration for Resource type  */
typedef ResourceManager<KNlpWrapper> KNlpResourceManager;

} // namespace sf1r

#endif // SF1R_RESOURCE_MANAGER_H
