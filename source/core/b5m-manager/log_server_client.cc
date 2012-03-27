#include "log_server_client.h"
#include "b5m_types.h"
#include "b5m_helper.h"
#include <common/ScdParser.h>
#include <common/Utilities.h>
#include <document-manager/Document.h>
#include <log-manager/LogServerRequest.h>
#include <am/sequence_file/ssfr.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>

using namespace sf1r;

bool LogServerClient::Init(const LogServerConnectionConfig& config)
{
    LogServerConnection& conn = LogServerConnection::instance();
    if(!conn.init(config))
    {
        return false;
    }
    return true;
}

bool LogServerClient::Test()
{
    LogServerConnection& conn = LogServerConnection::instance();
    if(!conn.testServer())
    {
        return false;
    }
    return true;
}

void LogServerClient::Update(const std::string& pid, const std::vector<std::string>& docid_list)
{
    LogServerConnection& conn = LogServerConnection::instance();
    UpdateUUIDRequest uuidReq;
    uuidReq.param_.uuid_ = B5MHelper::StringToUint128(pid);
    uuidReq.param_.docidList_.resize(docid_list.size());
    for (uint32_t i = 0; i < docid_list.size(); i++)
    {
        uuidReq.param_.docidList_[i] = B5MHelper::StringToUint128(docid_list[i]);
    }
    conn.asynRequest(uuidReq);
}

void LogServerClient::AppendUpdate(const std::string& pid, const std::vector<std::string>& docid_list)
{
    std::vector<std::string> list;
    GetDocidList(pid, list);
    list.insert(list.end(), docid_list.begin(), docid_list.end());
    Update(pid, list);
}

void LogServerClient::RemoveUpdate(const std::string& pid, const std::vector<std::string>& docid_list)
{
    std::vector<std::string> list;
    GetDocidList(pid, list);
    std::map<std::string, int> map;
    for(uint32_t i=0;i<list.size();i++)
    {
        map[list[i]] = 0;
    }
    for(uint32_t i=0;i<docid_list.size();i++)
    {
        map[docid_list[i]] = 1;
    }
    list.resize(0);
    for(std::map<std::string, int>::iterator it=map.begin();it!=map.end();it++)
    {
        if(it->second==0)
        {
            list.push_back(it->first);
        }
    }
    Update(pid, list);
}

bool LogServerClient::GetPid(const std::string& docid, std::string& pid)
{
    LogServerConnection& conn = LogServerConnection::instance();
    GetUUIDRequest uuidReq;
    Docid2UUID uuidResp;
    uuidReq.param_.docid_ = B5MHelper::StringToUint128(docid);
    conn.syncRequest(uuidReq, uuidResp);
    if (uuidReq.param_.docid_ != uuidResp.docid_)
    {
        return false;
    }
    pid = B5MHelper::Uint128ToString(uuidResp.uuid_);
    return true;
}
bool LogServerClient::GetDocidList(const std::string& pid, std::vector<std::string>& docid_list)
{
    LogServerConnection& conn = LogServerConnection::instance();
    GetDocidListRequest docidListReq;
    UUID2DocidList docidListResp;

    docidListReq.param_.uuid_ = B5MHelper::StringToUint128(pid);

    conn.syncRequest(docidListReq, docidListResp);
    if (docidListReq.param_.uuid_ != docidListResp.uuid_)
    {
        return false;
    }
    docid_list.resize(docidListResp.docidList_.size());
    for(uint32_t i=0;i<docid_list.size();i++)
    {
        docid_list[i] = B5MHelper::Uint128ToString(docidListResp.docidList_[i]);
    }
    return true;
}


void LogServerClient::Flush()
{
    LogServerConnection& conn = LogServerConnection::instance();
    SynchronizeRequest syncReq;
    conn.asynRequest(syncReq);
    conn.flushRequests();
}

