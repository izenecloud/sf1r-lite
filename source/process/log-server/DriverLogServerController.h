#ifndef DRIVER_LOG_SERVER_CONTROLLER_H_
#define DRIVER_LOG_SERVER_CONTROLLER_H_

#include "LogServerStorage.h"

#include <common/Keys.h>

#include <util/driver/Controller.h>
#include <util/driver/writers/JsonWriter.h>

#include <iostream>
#include <set>

namespace sf1r
{

/**
 * @brief Controller \b log_server
 * Log server for updating cclog
 */
class DriverLogServerController : public izenelib::driver::Controller
{
public:
    void init();

    void update_cclog();

    void setDriverCollections(const std::set<std::string>& collections)
    {
        driverCollections_ = collections;
    }

private:
    bool skipProcess(const std::string& collection);

    void processDocVisit(izenelib::driver::Request& request, const std::string& raw);

    void processRecVisitItem(izenelib::driver::Request& request, const std::string& raw);

    void processRecPurchaseItem(izenelib::driver::Request& request, const std::string& raw);

    void onUniqueKeyCheck(
            const uint128_t& uuid,
            const std::vector<uint32_t>& docidList,
            const std::string& aux);

    void onDuplicateKeyCheck(
            const uint128_t& uuid,
            const std::vector<uint32_t>& docidList,
            const std::string& aux);


private:
    izenelib::driver::JsonWriter jsonWriter_;

    std::set<std::string> driverCollections_;

    LogServerStorage::DrumPtr drum_;
    LogServerStorage::KVDBPtr docidDB_;
};

}

#endif /* DRIVER_LOG_SERVER_CONTROLLER_H_ */
