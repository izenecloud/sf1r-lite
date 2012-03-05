#include "log_server_handler.h"
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

LogServerHandler::LogServerHandler()
{
}

bool LogServerHandler::Send(const std::string& scd_path, const std::string& work_dir)
{
    namespace bfs = boost::filesystem;
    if(!bfs::exists(scd_path)) return false;
    std::vector<std::string> scd_list;
    if( bfs::is_regular_file(scd_path) && boost::algorithm::ends_with(scd_path, ".SCD"))
    {
        scd_list.push_back(scd_path);
    }
    else if(bfs::is_directory(scd_path))
    {
        bfs::path p(scd_path);
        bfs::directory_iterator end;
        for(bfs::directory_iterator it(p);it!=end;it++)
        {
            if(bfs::is_regular_file(it->path()))
            {
                std::string file = it->path().string();
                if(boost::algorithm::ends_with(file, ".SCD"))
                {
                    scd_list.push_back(file);
                }
            }
        }
    }
    if(scd_list.empty()) return false;
    typedef izenelib::util::UString UString;
    typedef boost::unordered_map<std::string, std::vector<std::string> > MapType;
    typedef std::pair<UString, UString> ValueType;
    MapType p2o_map;
    std::string working_dir = work_dir;
    if(working_dir.empty())
    {
        working_dir = "./b5m_log_server_working";
    }
    boost::filesystem::remove_all(working_dir);
    boost::filesystem::create_directories(working_dir);

    std::string writer_file = working_dir+"/writer";

    izenelib::am::ssf::Writer<> writer(writer_file);
    writer.Open();
    uint32_t n=0;
    for(uint32_t i=0;i<scd_list.size();i++)
    {
        std::string scd_file = scd_list[i];
        LOG(INFO)<<"Processing scd "<<scd_file<<std::endl;
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(scd_file);
        for( ScdParser::iterator doc_iter = parser.begin(B5MHelper::B5MO_PROPERTY_LIST);
          doc_iter!= parser.end(); ++doc_iter, ++n)
        {
            if(n%10000==0)
            {
                LOG(INFO)<<"Find Documents "<<n<<std::endl;
            }
            Document doc;
            SCDDoc& scddoc = *(*doc_iter);
            std::vector<std::pair<izenelib::util::UString, izenelib::util::UString> >::iterator p;
            for(p=scddoc.begin(); p!=scddoc.end(); ++p)
            {
                std::string property_name;
                p->first.convertString(property_name, izenelib::util::UString::UTF_8);
                doc.property(property_name) = p->second;
            }
            UString udocname;
            UString uuid;
            doc.getProperty("DOCID", udocname);
            doc.getProperty("uuid", uuid);
            if(udocname.length()==0 || uuid.length()==0 ) continue;
            uint64_t hash_id = izenelib::util::HashFunction<UString>::generateHash64(uuid);
            writer.Append(hash_id, std::make_pair(uuid, udocname));
        }
    }
    writer.Close();
    izenelib::am::ssf::Sorter<uint32_t, uint64_t>::Sort(writer_file);
    LogServerConnection& conn = LogServerConnection::instance();
    if(!conn.init(logserver_config_))
    {
        std::cout<<"log server init failed"<<std::endl;
        return false;
    }
    izenelib::am::ssf::Joiner<uint32_t, uint64_t, ValueType> joiner(writer_file);
    joiner.Open();
    uint64_t key;
    std::vector<ValueType> values;
    n = 0;
    while(joiner.Next(key, values))
    {
        ++n;
        if(n%100000==0)
        {
            LOG(INFO)<<"Process "<<n<<" values"<<std::endl;
        }
        if(values.empty()) continue;
        std::string uuidstr;
        values[0].first.convertString(uuidstr, izenelib::util::UString::UTF_8);
        UpdateUUIDRequest uuidReq;
        uuidReq.param_.uuid_ = Utilities::uuidToUint128(uuidstr);
        for (uint32_t i = 0; i < values.size(); i++)
        {
            std::string docname;
            values[i].second.convertString(docname, izenelib::util::UString::UTF_8);
            uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128(docname));
        }
        conn.asynRequest(uuidReq);
    }
    SynchronizeRequest syncReq;
    conn.asynRequest(syncReq);
    boost::filesystem::remove_all(working_dir);
    return true;
}

bool LogServerHandler::QuickSend(const std::string& scd_path)
{
    namespace bfs = boost::filesystem;
    if(!bfs::exists(scd_path)) return false;
    std::vector<std::string> scd_list;
    if( bfs::is_regular_file(scd_path) && boost::algorithm::ends_with(scd_path, ".SCD"))
    {
        scd_list.push_back(scd_path);
    }
    else if(bfs::is_directory(scd_path))
    {
        bfs::path p(scd_path);
        bfs::directory_iterator end;
        for(bfs::directory_iterator it(p);it!=end;it++)
        {
            if(bfs::is_regular_file(it->path()))
            {
                std::string file = it->path().string();
                if(boost::algorithm::ends_with(file, ".SCD"))
                {
                    scd_list.push_back(file);
                }
            }
        }
    }
    if(scd_list.empty()) return false;
    typedef izenelib::util::UString UString;

    uint32_t n=0;
    std::string suuid;
    std::vector<std::string> sdocname_list;
    LogServerConnection& conn = LogServerConnection::instance();
    if(!conn.init(logserver_config_))
    {
        std::cout<<"log server init failed"<<std::endl;
        return false;
    }
    for(uint32_t i=0;i<scd_list.size();i++)
    {
        std::string scd_file = scd_list[i];
        LOG(INFO)<<"Processing scd "<<scd_file<<std::endl;
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(scd_file);
        for( ScdParser::iterator doc_iter = parser.begin(B5MHelper::B5MO_PROPERTY_LIST);
          doc_iter!= parser.end(); ++doc_iter, ++n)
        {
            if(n%10000==0)
            {
                LOG(INFO)<<"Find Documents "<<n<<std::endl;
            }
            Document doc;
            SCDDoc& scddoc = *(*doc_iter);
            std::vector<std::pair<izenelib::util::UString, izenelib::util::UString> >::iterator p;
            for(p=scddoc.begin(); p!=scddoc.end(); ++p)
            {
                std::string property_name;
                p->first.convertString(property_name, izenelib::util::UString::UTF_8);
                doc.property(property_name) = p->second;
            }
            UString udocname;
            UString uuid;
            doc.getProperty("DOCID", udocname);
            doc.getProperty("uuid", uuid);
            if(udocname.length()==0 || uuid.length()==0 ) continue;
            std::string str_uuid;
            std::string sdocname;
            uuid.convertString(str_uuid, UString::UTF_8);
            udocname.convertString(sdocname, UString::UTF_8);
            if(str_uuid!=suuid && !sdocname_list.empty())
            {
                Post_(conn, suuid, sdocname_list);
                sdocname_list.resize(0);
            }
            suuid = str_uuid;
            sdocname_list.push_back(sdocname);
        }
    }
    if(!sdocname_list.empty())
    {
        Post_(conn, suuid, sdocname_list);
        sdocname_list.resize(0);
    }
    SynchronizeRequest syncReq;
    conn.asynRequest(syncReq);
    return true;
}

void LogServerHandler::Post_(LogServerConnection& conn, const std::string& suuid, const std::vector<std::string>& sdocname_list)
{
    UpdateUUIDRequest uuidReq;
    uuidReq.param_.uuid_ = Utilities::uuidToUint128(suuid);
    for (uint32_t i = 0; i < sdocname_list.size(); i++)
    {
        uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128(sdocname_list[i]));
    }
    conn.asynRequest(uuidReq);
}

