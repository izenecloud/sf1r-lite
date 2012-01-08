#ifndef DRIVER_LOG_SERVER_CONTROLLER_H_
#define DRIVER_LOG_SERVER_CONTROLLER_H_

#include "LogServerStorage.h"

#include <common/Keys.h>

#include <util/driver/Controller.h>
#include <util/driver/writers/JsonWriter.h>

#include <iostream>
#include <set>

using namespace izenelib::driver;

namespace sf1r
{

/**
 * @brief Controller \b log_server
 * Log server for updating cclog
 */
class DriverLogServerHandler;
class DriverLogServerController : public izenelib::driver::Controller
{
public:
    DriverLogServerController(const boost::shared_ptr<DriverLogServerHandler>& driverLogServerHandler)
    {
        dirverLogServerHandler_ = driverLogServerHandler;
        BOOST_ASSERT(dirverLogServerHandler_);
    }

    /// overwrite
    bool preprocess();

    void update_cclog();

private:
    boost::shared_ptr<DriverLogServerHandler> dirverLogServerHandler_;
};

class DriverLogServerHandler
{
public:
    void setRequestContext(
            Request& request,
            Response& response)
    {
        request_ = &request;
        response_ = &response;
    }

    Request& request()
    {
        return *request_;
    }

    const Request& request() const
    {
        return *request_;
    }

    Response& response()
    {
        return *response_;
    }

    const Response& response() const
    {
        return *response_;
    }

public:
    void init();

    void process();

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

    void ouputCclog(const std::string& log);

    void mergeCClog();

private:
    Request* request_;
    Response* response_;
    izenelib::driver::JsonWriter jsonWriter_;

    std::set<std::string> driverCollections_;

    LogServerStorage::DrumPtr drum_;
    LogServerStorage::KVDBPtr docidDB_;

    std::string cclogFileName_;
    boost::shared_ptr<std::ofstream> cclogFile_;
    boost::mutex mutex_;

    struct CCLogMerge
    {
        bool merged_;
        std::string request_;
        std::size_t uuidCnt_;
        std::vector<std::pair<uint128_t, std::set<uint128_t> > > uuidUpdateVec_;

        CCLogMerge()
        : merged_(false)
        , uuidCnt_(0)
        {}
    };
    std::map<uint128_t, boost::shared_ptr<CCLogMerge> > cclogMergeQueue_;
    boost::mutex cclog_merge_mutex_;
};

}

#endif /* DRIVER_LOG_SERVER_CONTROLLER_H_ */
