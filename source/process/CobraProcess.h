#ifndef COBRAPROCESS_H_
#define COBRAPROCESS_H_

#include <distribute/MasterServer.h>
#include <util/driver/DriverServer.h>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <string>

class CobraProcess
{
public:
    CobraProcess() {}

    int run();

    bool initialize(const std::string& configFileDir);

private:
    bool initLogManager();

    bool initLicenseManager();

    bool initLAManager();

    void initQuery();

    bool initFireWall();

    bool initDriverServer();

    bool initNodeManager();

    void stopDriver();

    bool startDistributedServer();

    void stopDistributedServer();

    void scheduleTask(const std::string& collection);

private:
    std::string configDir_;

    boost::scoped_ptr<izenelib::driver::DriverServer> driverServer_;
    boost::scoped_ptr<sf1r::MasterServer> masterServer_;

    boost::shared_ptr<izenelib::driver::Router> router_;
};

#endif /*COBRAPROCESS_H_*/
