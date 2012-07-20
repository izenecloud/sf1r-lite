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
        std::cout << "log server test failed!" << endl;
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

// [docid: ..., oldpid]
bool LogServerClient::AddOldUUID(const std::string& docid, const std::string& oldpid)
{
    if(docid.size() < 32 || oldpid.size() < 32)
    {
        std::cout << "error length for the docid or old pid" << endl;
        return false;
    }
    LogServerConnection& conn = LogServerConnection::instance();
    AddOldUUIDRequest add_uuidreq;
    add_uuidreq.param_.docid_ =  Utilities::uuidToUint128(docid);
    add_uuidreq.param_.olduuid_ = oldpid;
    OldUUIDData rsp;
    conn.syncRequest(add_uuidreq, rsp);
    return true;
}
// [pid: ..., olddocid]
bool LogServerClient::AddOldDocId(const std::string& pid, const std::string& olddocid)
{
    if(pid.size() < 32 || olddocid.size() < 32)
    {
        std::cout << "error length for the docid or old pid" << endl;
        return false;
    }
    LogServerConnection& conn = LogServerConnection::instance();
    AddOldDocIdRequest add_docidreq;
    add_docidreq.param_.uuid_ = Utilities::uuidToUint128(pid);
    add_docidreq.param_.olddocid_ = olddocid;
    OldDocIdData rsp;
    conn.syncRequest(add_docidreq, rsp);
    return true;
}
// remove offer id from all history products when an offer was deleted.
bool LogServerClient::DelOldDocId(const std::vector<std::string>& pids, const std::string& olddocid)
{
    LogServerConnection& conn = LogServerConnection::instance();
    std::vector<uint128_t>  oldpid_vec;
    for(size_t i = 0; i < pids.size(); ++i)
    {
        if(pids.empty() || pids.size() < 32) continue;
        cout << pids[i] << endl;
        oldpid_vec.push_back(Utilities::uuidToUint128(pids[i]));
    }
    DelOldDocIdRequest del_docidreq;
    del_docidreq.param_.uuid_list_ = oldpid_vec;
    del_docidreq.param_.olddocid_ = olddocid;
    DelOldDocIdData rsp;
    conn.syncRequest(del_docidreq, rsp);
    return true;
}

bool LogServerClient::GetOldDocIdList(const std::string& pid, std::vector<std::string>& olddocid_list)
{
    std::string olddocid_str;
    bool ret = GetOldDocIdList(pid, olddocid_str);
    boost::algorithm::split(olddocid_list, olddocid_str, boost::is_any_of(","));
    return ret;
}

bool LogServerClient::GetOldDocIdList(const std::string& pid, std::string& olddocid_list_str)
{
    LogServerConnection& conn = LogServerConnection::instance();
    GetOldDocIdRequest get_docidreq;
    get_docidreq.param_.uuid_ = Utilities::uuidToUint128(pid);
    OldDocIdData rsp;
    conn.syncRequest(get_docidreq, rsp);
    olddocid_list_str = rsp.olddocid_;
    return rsp.success_;
}

bool LogServerClient::GetOldUUIDList(const std::string& docid, std::string& oldpid_list_str)
{
    LogServerConnection& conn = LogServerConnection::instance();
    GetOldUUIDRequest get_uuidreq;
    get_uuidreq.param_.docid_ = Utilities::uuidToUint128(docid);
    OldUUIDData rsp;
    conn.syncRequest(get_uuidreq, rsp);
    oldpid_list_str = rsp.olduuid_;
    return rsp.success_;
}

bool LogServerClient::GetOldUUIDList(const std::string& docid, std::vector<std::string>& oldpid_list)
{
    std::string olduuid_str;
    bool ret = GetOldUUIDList(docid, olduuid_str);
    boost::algorithm::split(oldpid_list, olduuid_str, boost::is_any_of(","));
    return ret;
}

void LogServerClient::Flush()
{
    LogServerConnection& conn = LogServerConnection::instance();
    SynchronizeRequest syncReq;
    conn.asynRequest(syncReq);
    conn.flushRequests();
}
