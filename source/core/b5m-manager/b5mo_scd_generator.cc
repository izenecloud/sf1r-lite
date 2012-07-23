#include "b5mo_scd_generator.h"
#include "log_server_client.h"
#include "b5m_types.h"
#include "b5m_helper.h"
#include <common/ScdParser.h>
#include <common/ScdWriter.h>
#include <common/ScdWriterController.h>
#include <common/Utilities.h>
#include <product-manager/product_price.h>
#include <product-manager/uuid_generator.h>
#include <configuration-manager/LogServerConnectionConfig.h>
#include <am/sequence_file/ssfr.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>

using namespace sf1r;

B5moScdGenerator::B5moScdGenerator(OfferDb* odb, B5MHistoryDBHelper* hdb, LogServerConnectionConfig* config)
:odb_(odb), historydb_(hdb), log_server_cfg_(config)
{
}

//bool B5moScdGenerator::Load_(const std::string& category_dir)
//{
    //LOG(INFO)<<"Loading "<<category_dir<<std::endl;
    //if(!boost::filesystem::exists(category_dir)) return false;
    //{
        //std::string regex_file = category_dir+"/category";
        //std::ifstream ifs(regex_file.c_str());
        //std::string line;
        //if( getline(ifs, line) )
        //{
            //boost::algorithm::trim(line);
            //boost::regex r(line);
            //category_regex_.push_back(r);
        //}
        //ifs.close();
    //}
    //{
        //std::string match_file = category_dir+"/match";
        //std::ifstream ifs(match_file.c_str());
        //std::string line;
        //while( getline(ifs,line))
        //{
            //boost::algorithm::trim(line);
            //std::vector<std::string> vec;
            //boost::algorithm::split(vec, line, boost::is_any_of(","));
            //if(vec.size()<2) continue;
            //o2p_map_.insert(std::make_pair(vec[0], vec[1]));
        //}
        //ifs.close();
    //}
    //return true;
//}

//bool B5moScdGenerator::Load(const std::string& dir)
//{
    //namespace bfs = boost::filesystem;
    //if(!bfs::exists(dir)) return false;
    //std::string match_file = dir+"/match";
    //std::string category_file = dir+"/category";
    //if(bfs::exists(match_file))
    //{
        //Load_(dir);
    //}
    //else if(!bfs::exists(category_file))
    //{
        //bfs::path p(dir);
        //bfs::directory_iterator end;
        //for(bfs::directory_iterator it(p);it!=end;it++)
        //{
            //if(bfs::is_directory(it->path()))
            //{
                //Load_(it->path().string());
            //}
        //}
    //}
    //return true;
//}

bool B5moScdGenerator::Generate(const std::string& mdb_instance)
{
    if(!odb_->is_open())
    {
        if(!odb_->open())
        {
            LOG(ERROR)<<"odb open fail"<<std::endl;
            return false;
        }
    }
    if(historydb_)
    {
        if(!historydb_->is_open())
        {
            if(!historydb_->open())
            {
                LOG(ERROR)<<"B5MHistoryDBHelper open fail"<<std::endl;
                return false;
            }
        }
    }
    else
    {
        if(log_server_cfg_ == NULL)
            return false;
        // use logserver instead of local history 
        if(!LogServerClient::Init(*log_server_cfg_))
        {
            LOG(ERROR) << "Log Server Init failed." << std::endl;
            return false;
        }
        if(!LogServerClient::Test())
        {
            LOG(ERROR) << "log server test failed" << std::endl;
            return false;
        }
    }

    //boost::filesystem::path mi_path(mdb_instance);
    //std::string ts = mi_path.filename();
    std::string scd_path = B5MHelper::GetRawPath(mdb_instance);
    std::string output_dir = B5MHelper::GetB5moPath(mdb_instance);
    typedef izenelib::util::UString UString;
    namespace bfs = boost::filesystem;
    B5MHelper::PrepareEmptyDir(output_dir);

    std::string match_file = mdb_instance+"/match";
    if(bfs::exists(match_file))
    {
        LOG(INFO)<<"odb loading match file "<<match_file<<std::endl;
        odb_->load_text(match_file);
        if(!odb_->flush())
        {
            LOG(ERROR)<<"odb flush error"<<std::endl;
        }
    }
    std::vector<std::string> scd_list;
    B5MHelper::GetScdList(scd_path, scd_list);
    if(scd_list.empty()) return false;

    ScdWriter b5mo_i(output_dir, INSERT_SCD);
    ScdWriter b5mo_u(output_dir, UPDATE_SCD);
    ScdWriter b5mo_d(output_dir, DELETE_SCD);

    for(uint32_t i=0;i<scd_list.size();i++)
    {
        std::string scd_file = scd_list[i];
        LOG(INFO)<<"Processing "<<scd_file<<std::endl;
        int scd_type = ScdParser::checkSCDType(scd_file);
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(scd_file);
        uint32_t n=0;
        for( ScdParser::iterator doc_iter = parser.begin();
          doc_iter!= parser.end(); ++doc_iter, ++n)
        {
            if(n%10000==0)
            {
                LOG(INFO)<<"Find Documents "<<n<<std::endl;
            }
            std::string soldpid;
            Document doc;
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
                doc.property(property_name) = p->second;
                if(property_name ==  "uuid")
                {
                    p->second.convertString(soldpid, UString::UTF_8);
                }
            }
            std::string sdocid;
            doc.getString("DOCID", sdocid);
            //doc.property("lastmodified") = UString(ts_, UString::UTF_8);
            std::string spid;
            if(!odb_->get(sdocid, spid))
            {
                spid = sdocid;
                odb_->insert(sdocid, spid);
            }
            doc.property("uuid") = UString(spid, UString::UTF_8);
            std::string olduuid;
            doc.getString("olduuid", olduuid);
            
            if(scd_type==INSERT_SCD)
            {
                b5mo_i.Append(doc);
            }
            else if(scd_type==UPDATE_SCD)
            {
                if(!soldpid.empty() && soldpid != spid)
                {
                    if(olduuid.empty())
                    {
                        olduuid = soldpid;
                    }
                    else
                    {
                        olduuid += "," + soldpid;
                    }
                    LOG(INFO)<<"offer's pid has changed from " << soldpid << " to " << spid << std::endl;
                    doc.property("olduuid") = UString(olduuid, UString::UTF_8);
                    if(historydb_)
                    {
                        historydb_->offer_insert(sdocid, soldpid);
                        historydb_->pd_insert(soldpid, sdocid);
                    }
                    else
                    {
                        LogServerClient::AddOldUUID(sdocid, soldpid);
                        LogServerClient::AddOldDocId(soldpid, sdocid);
                    }
                }
                b5mo_u.Append(doc);
            }
            else if(scd_type==DELETE_SCD)
            {
                b5mo_d.Append(doc);
                if(historydb_)
                {
                    std::set<std::string> s_oldpids;
                    historydb_->pd_remove_offerid(spid, sdocid);
                    // delete the offerid from all its history product ids
                    historydb_->offer_get(sdocid, s_oldpids);
                    std::set<std::string>::iterator it = s_oldpids.begin();
                    std::set<std::string>::iterator it_end = s_oldpids.end();
                    while(it != it_end)
                    {
                        if((*it).empty()) continue;
                        historydb_->pd_remove_offerid(*it, sdocid);
                        ++it;
                    }
                }
                else
                {
                    std::vector<std::string> s_oldpids;
                    LogServerClient::GetOldUUIDList(sdocid, s_oldpids);
                    s_oldpids.push_back(spid);
                    LogServerClient::DelOldDocId(s_oldpids, sdocid);
                }
            }
        }
    }
    if(!odb_->flush())
    {
        LOG(WARNING)<<"odb flush error"<<std::endl;
    }
    if(historydb_)
    {
        if(!historydb_->flush())
        {
            LOG(WARNING)<<"B5MHistoryDBHelper flush error"<<std::endl;
        }
    }
    else
    {
        LogServerClient::Flush();
    }
 
    b5mo_i.Close();
    b5mo_u.Close();
    b5mo_d.Close();
    return true;
}

