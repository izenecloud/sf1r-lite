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

LogServerHandler::LogServerHandler(const LogServerConnectionConfig& config)
:logserver_config_(config)
{
}

bool LogServerHandler::Generate(const std::string& mdb_instance)
{
    if(!LogServerClient::Init(logserver_config_))
    {
        LOG(ERROR)<<"log server init failed"<<std::endl;
        return false;
    }
    if(!LogServerClient::Test())
    {
        LOG(ERROR)<<"log server test connection failed"<<std::endl;
        return false;
    }

    
    std::string po_map_file = B5MHelper::GetPoMapPath(mdb_instance);
    if(!boost::filesystem::exists(po_map_file))
    {
        LOG(ERROR)<<"po map file not exists, run b5mp-generate first."<<std::endl;
        return false;
    }
    izenelib::am::ssf::Sorter<uint32_t, uint128_t>::Sort(po_map_file);
    izenelib::am::ssf::Joiner<uint32_t, uint128_t, std::string> joiner(po_map_file);
    joiner.Open();
    std::vector<uint128_t> ioid_list;
    std::vector<std::string> oid_list;
    uint128_t ipid;
    std::string pid;
    std::size_t n = 0;
    while(joiner.Next(ipid, oid_list))
    {
        ++n;
        if(n%10000==0)
        {
            LOG(INFO)<<"update "<<n<<" pid"<<std::endl;
        }
        pid = B5MHelper::Uint128ToString(ipid);
        std::vector<std::string> non_empty_oid_list;
        for(uint32_t i=0;i<oid_list.size();i++)
        {
            if(!oid_list[i].empty()) non_empty_oid_list.push_back(oid_list[i]);
        }
        LogServerClient::Update(pid, non_empty_oid_list);
    }
    LOG(INFO)<<"log server start flushing"<<std::endl;
    LogServerClient::Flush();
    LOG(INFO)<<"log server flush finished"<<std::endl;
    return true;
}

//bool LogServerHandler::Generate(const std::string& mdb_instance)
//{
    //if(!LogServerClient::Init(logserver_config_))
    //{
        //LOG(ERROR)<<"log server init failed"<<std::endl;
        //return false;
    //}
    //if(!LogServerClient::Test())
    //{
        //LOG(ERROR)<<"log server test connection failed"<<std::endl;
        //return false;
    //}
    //if(!odb_->is_open())
    //{
        //if(!odb_->open())
        //{
            //LOG(ERROR)<<"open odb fail"<<std::endl;
            //return false;
        //}
    //}
    //boost::unordered_set<uint128_t, uint128_hash> pid_change;

    //std::vector<std::string> scd_list;
    //B5MHelper::GetScdList(B5MHelper::GetB5mpPath(mdb_instance), scd_list);
    //if(scd_list.empty())
    //{
        //LOG(WARNING)<<"no b5mp scd"<<std::endl;
        //return true;
    //}

    //for(uint32_t i=0;i<scd_list.size();i++)
    //{
        //std::string scd_file = scd_list[i];
        //LOG(INFO)<<"Processing "<<scd_file<<std::endl;
        //int scd_type = ScdParser::checkSCDType(scd_file);
        //ScdParser parser(izenelib::util::UString::UTF_8);
        //parser.load(scd_file);
        //uint32_t n=0;
        //for( ScdParser::iterator doc_iter = parser.begin();
          //doc_iter!= parser.end(); ++doc_iter, ++n)
        //{
            //if(n%10000==0)
            //{
                //LOG(INFO)<<"Find Documents "<<n<<std::endl;
            //}
            //UString pid;
            //SCDDoc& scddoc = *(*doc_iter);
            //SCDDoc::iterator p = scddoc.begin();
            //for(; p!=scddoc.end(); ++p)
            //{
                //const std::string& property_name = p->first;
                //if(property_name=="DOCID")
                //{
                    //pid = p->second;
                    //break;
                //}
            //}
            //if(pid.empty()) continue;
            //uint128_t ipid = B5MHelper::UStringToUint128(pid);
            //pid_change.insert(ipid);
        //}
    //}
    //std::string work_path = work_dir_;
    //if(work_path.empty())
    //{
        //work_path = "./logserver-update-working";
    //}
    //B5MHelper::PrepareEmptyDir(work_path);
    //izenelib::am::ssf::Writer<> writer(work_path+"/writer");
    //writer.Open();
    //uint64_t n=0;
    //for(OfferDb::const_iterator it = odb_->begin(); it!=odb_->end(); ++it, ++n)
    //{
        //if(n%1000000==0)
        //{
            //LOG(INFO)<<"iter "<<n<<" in odb"<<std::endl;
        //}
        //if(pid_change.find(it->second) == pid_change.end()) continue;
        //writer.Append( it->second, B5MHelper::Uint128ToString(it->first));
    //}
    //writer.Close();
    //izenelib::am::ssf::Sorter<uint32_t, uint128_t>::Sort(writer.GetPath());
    //izenelib::am::ssf::Joiner<uint32_t, uint128_t, std::string> joiner(writer.GetPath());
    //joiner.Open();
    //std::vector<uint128_t> ioid_list;
    //std::vector<std::string> oid_list;
    //uint128_t ipid;
    //std::string pid;
    //n = 0;
    //while(joiner.Next(ipid, oid_list))
    //{
        //pid = B5MHelper::Uint128ToString(ipid);
        ////std::cout<<"find pid "<<pid<<std::endl;
        //++n;
        //if(n%10000==0)
        //{
            //LOG(INFO)<<"update "<<n<<" pid"<<std::endl;
        //}
        //LogServerClient::Update(pid, oid_list);
    //}
    //LogServerClient::Flush();
    //boost::filesystem::remove_all(work_path);
    //return true;
//}

