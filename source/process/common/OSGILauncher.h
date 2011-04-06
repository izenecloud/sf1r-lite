#ifndef OSGI_LAUNCHER_H
#define OSGI_LAUNCHER_H

#include <util/osgi/Launcher.h>
#include <util/ThreadModel.h>

#include <boost/shared_ptr.hpp>
#include <string>

namespace sf1r
{
using namespace izenelib::osgi;
class OSGILauncher : public Launcher<RecursiveLock>
{
public:
    OSGILauncher();

    virtual ~OSGILauncher();

    void start(boost::shared_ptr<BundleConfiguration> bundleConfig);

    IService* getService(const std::string& bundleName, const std::string& serviceName);

    void stop();
};

}

#endif

