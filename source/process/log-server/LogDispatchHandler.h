#ifndef LOG_DISPATCH_HANDLER_H_
#define LOG_DISPATCH_HANDLER_H_

#include "LogServerStorage.h"

#include <common/Keys.h>
#include <util/driver/Controller.h>
#include <util/driver/writers/JsonWriter.h>
#include <net/sf1r/Sf1Driver.hpp>

using namespace izenelib::net::sf1r;
using namespace izenelib::driver;

namespace sf1r
{

/***********************************************
*@brief LogDispatchHandler is used to send specified requests
*  to SF1, to recover user behavior data from logs
************************************************/
class LogDispatchHandler
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
    void Init();

    void DispatchLog();

    void BackupRawCclog();

    void ConvertRawCclog();

    void UpdateSCD();

    void UpdateDocuments();

    void Flush();

private:
    bool SkipProcess_(const std::string& collection);

    void DocVisit_(izenelib::driver::Value& request);

    void RecVisitItem_(izenelib::driver::Value& request);

    void RecPurchaseItem_(izenelib::driver::Value& request);

    void RecPurchaseItem_(izenelib::driver::Value& request, const std::string& raw);

    /// backup uuid to raw docid
    void DocVisitBackupRawid_(izenelib::driver::Value& request);

    void RecVisitItemBackupRawid_(izenelib::driver::Value& request);

    void RecPurchaseItemBackupRawid_(izenelib::driver::Value& request);

    /// convert raw docid to latest uuid
    bool DocVisitConvertRawid_(izenelib::driver::Value& request);

    bool RecVisitItemConvertRawid_(izenelib::driver::Value& request);

    bool RecPurchaseItemConvertRawid_(izenelib::driver::Value& request);

    std::string UpdateUuidStr_(const std::string& uuidStr);

    bool GetUuidByDocidList_(const std::vector<uint128_t>& docidList, std::vector<uint128_t>& uuids);

    /// reserved
    void OnUniqueKeyCheck(
        const LogServerStorage::uuid_t& uuid,
        const LogServerStorage::raw_docid_list_t& docidList,
        const std::string& aux);

    /// reserved
    void OnDuplicateKeyCheck(
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
    void MergeCClog_(boost::shared_ptr<CCLogMerge>& cclogMergeUnit);

    struct OutFile
    {
        boost::shared_ptr<std::ofstream> of_;
        boost::mutex mutex_;
    };

    boost::shared_ptr<OutFile>& OpenFile_(const std::string& fileName);

    /// reserved
    void EncodeFileName_(std::string& json, const std::string& fileName);

    /// reserved
    void DecodeFileName_(std::string& json, std::string& fileName);

    void InitSf1DriverClient_(const std::string& host, uint32_t port);

    void OutputCclog_(const std::string& fileName, const std::string& uri, const std::string& request);

    void WriteFile_(const std::string& fileName, const std::string& line);

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

    boost::shared_ptr<Sf1Driver> sf1DriverClient_;
    std::string sf1Host_;
    uint32_t sf1Port_;
};

}

#endif

