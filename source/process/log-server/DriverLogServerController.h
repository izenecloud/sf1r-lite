#ifndef DRIVER_LOG_SERVER_CONTROLLER_H_
#define DRIVER_LOG_SERVER_CONTROLLER_H_

#include "LogServerStorage.h"

#include <common/Keys.h>

#include <util/driver/Controller.h>
#include <util/driver/writers/JsonWriter.h>

#include <net/sf1r/Sf1Driver.hpp>

#include <iostream>
#include <set>

using namespace izenelib::driver;
using namespace izenelib::net::sf1r;

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

    void backup_raw_cclog();

    void convert_raw_cclog();

    void update_scd(); // reserved

    void update_documents();

    void flush();

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

    void processCclog();

    void processBackupRawCclog();

    void processConvertRawCclog();

    void processScd();

    void processUpdateDocuments();

    void flush();

private:
    bool skipProcess(const std::string& collection);

    void processDocVisit(izenelib::driver::Value& request);

    void processRecVisitItem(izenelib::driver::Value& request);

    void processRecPurchaseItem(izenelib::driver::Value& request);

    void processRecPurchaseItem(izenelib::driver::Value& request, const std::string& raw);

    /// backup uuid to raw docid
    void processDocVisitBackupRawid(izenelib::driver::Value& request);

    void processRecVisitItemBackupRawid(izenelib::driver::Value& request);

    void processRecPurchaseItemBackupRawid(izenelib::driver::Value& request);

    /// convert raw docid to latest uuid
    bool processDocVisitConvertRawid(izenelib::driver::Value& request);

    bool processRecVisitItemConvertRawid(izenelib::driver::Value& request);

    bool processRecPurchaseItemConvertRawid(izenelib::driver::Value& request);

    std::string updateUuidStr(const std::string& uuidStr);

    bool getUuidByDocidList(const std::vector<uint128_t>& docidList, std::vector<uint128_t>& uuids);

    /// reserved
    void onUniqueKeyCheck(
            const LogServerStorage::uuid_t& uuid,
            const LogServerStorage::raw_docid_list_t& docidList,
            const std::string& aux);

    /// reserved
    void onDuplicateKeyCheck(
            const LogServerStorage::uuid_t& uuid,
            const LogServerStorage::raw_docid_list_t& docidList,
            const std::string& aux);

    struct CCLogMerge
    {
        std::size_t uuidCnt_;
        std::string request_;
        std::vector<std::pair<uint128_t, std::set<uint128_t> > > uuidUpdateVec_;

        CCLogMerge(std::size_t uuidCnt = 0, const std::string& request = "")
            : uuidCnt_(uuidCnt)
            , request_(request)
        {}
    };

    /// reserved
    void mergeCClog(boost::shared_ptr<CCLogMerge>& cclogMergeUnit);

    struct OutFile
    {
        boost::shared_ptr<std::ofstream> of_;
        boost::mutex mutex_;
    };

    boost::shared_ptr<OutFile>& openFile(const std::string& fileName);

    /// reserved
    void encodeFileName(std::string& json, const std::string& fileName);

    /// reserved
    void decodeFileName(std::string& json, std::string& fileName);

    void setCclogSf1DriverClient(const std::string& host, uint32_t port);
    boost::shared_ptr<Sf1Driver>& getCclogSf1DriverClient();

    void outputCclog(const std::string& fileName, const std::string& uri, const std::string& request);

    void writeFile(const std::string& fileName, const std::string& line);

private:
    Request* request_;
    Response* response_;
    izenelib::driver::JsonWriter jsonWriter_;

    std::set<std::string> driverCollections_;
    std::string storageBaseDir_;

    typedef std::map<std::string, boost::shared_ptr<OutFile> > FileMapT;
    FileMapT fileMap_;

    std::map<uint128_t, boost::shared_ptr<CCLogMerge> > cclogMergeQueue_;
    boost::mutex cclog_merge_mutex_;

    // for filtering requests
    std::map<std::string, bool> cclogRequestMap_;

    boost::shared_ptr<Sf1Driver> cclogSf1DriverClient_;
    std::string cclogSf1Host_;
    uint32_t cclogSf1Port_;
};

}

#endif /* DRIVER_LOG_SERVER_CONTROLLER_H_ */
