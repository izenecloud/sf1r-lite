#include "log_server_handler.h"
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

LogServerHandler::LogServerHandler(const LogServerConnectionConfig& config, OfferDb* odb, const std::string& work_dir)
:logserver_config_(config), odb_(odb), work_dir_(work_dir), open_(false)
{
}

bool LogServerHandler::Open()
{
    if(open_) return true;
    if(!LogServerClient::Init(logserver_config_))
    {
        std::cout<<"log server init failed"<<std::endl;
        return false;
    }
    if(!LogServerClient::Test())
    {
        std::cout<<"log server test connection failed"<<std::endl;
        return false;
    }
    if(!odb_->simple_open())
    {
        std::cout<<"simple open odb fail"<<std::endl;
        return false;
    }
    open_ = true;
    return true;
}

void LogServerHandler::Process(const BuueItem& item)
{
    changed_pid_.insert(item.pid);
    //if(item.type==BUUE_APPEND)
    //{
        //if(item.pid.length()>0)
        //{
            //if(reindex_)
            //{
                //LogServerClient::Update(item.pid, item.docid_list);
            //}
            //else
            //{
                //LogServerClient::AppendUpdate(item.pid, item.docid_list);
            //}

        //}
    //}
    //else if(item.type==BUUE_REMOVE)
    //{
        //if(!reindex_)
        //{
            ////if(uuid=="*")
            ////{
                ////if(!LogServerClient::GetUuid(item.docid_list[0], uuid)) return;
            ////}
            //LogServerClient::RemoveUpdate(item.pid, item.docid_list);
        //}

    //}
}

void LogServerHandler::Finish()
{
    LOG(INFO)<<"LogServerHandler::Finish"<<std::endl;
    std::string work_path = work_dir_;
    if(work_path.empty())
    {
        work_path = "./logserver-update-working";
    }
    OfferDb::cursor_type it = odb_->begin();
    OfferDb::KeyType oid;
    OfferDb::ValueType ovalue;
    boost::filesystem::remove_all(work_path);
    boost::filesystem::create_directories(work_path);
    izenelib::am::ssf::Writer<> writer(work_path+"/writer");
    writer.Open();
    uint64_t n=0;
    while(true)
    {
        ++n;
        if(!odb_->fetch(it, oid, ovalue)) break;
        odb_->iterNext(it);
        //LOG(INFO)<<"find pid "<<pid<<std::endl;
        if(n%1000000==0)
        {
            LOG(INFO)<<"iter "<<n<<" in odb"<<std::endl;
        }
        if(changed_pid_.find(ovalue.pid) == changed_pid_.end()) continue;
        //writer.Append( B5MHelper::StringToUint128(pid), B5MHelper::StringToUint128(oid));
        writer.Append( B5MHelper::StringToUint128(ovalue.pid), oid);
    }
    writer.Close();
    izenelib::am::ssf::Sorter<uint32_t, uint128_t>::Sort(writer.GetPath());
    //izenelib::am::ssf::Joiner<uint32_t, uint128_t, uint128_t> joiner(writer.GetPath());
    izenelib::am::ssf::Joiner<uint32_t, uint128_t, std::string> joiner(writer.GetPath());
    joiner.Open();
    std::vector<uint128_t> ioid_list;
    std::vector<std::string> oid_list;
    uint128_t ipid;
    std::string pid;
    n = 0;
    while(joiner.Next(ipid, oid_list))
    {
        pid = B5MHelper::Uint128ToString(ipid);
        //std::cout<<"find pid "<<pid<<std::endl;
        ++n;
        if(n%10000==0)
        {
            LOG(INFO)<<"update "<<n<<" pid"<<std::endl;
        }
        //LogServerClient::Update(ipid, ioid_list);
        LogServerClient::Update(pid, oid_list);
    }
    LogServerClient::Flush();
    boost::filesystem::remove_all(work_path);
}

