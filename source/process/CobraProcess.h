#ifndef COBRAPROCESS_H_
#define COBRAPROCESS_H_

#include <bundles/querylog/QueryLogSearchService.h>

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
    bool initLicenseManager();

    bool initLAManager();

    QueryLogSearchService* initQuery();

    bool initFireWall();

    bool initDriverServer();

private:
    std::string configDir_;

    boost::scoped_ptr<izenelib::driver::DriverServer> driverServer_;

    boost::shared_ptr<izenelib::driver::Router> router_;

};

#endif /*COBRAPROCESS_H_*/
