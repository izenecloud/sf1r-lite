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

B5moScdGenerator::B5moScdGenerator(OfferDb* odb)
:odb_(odb)
{
}


bool B5moScdGenerator::Generate(const std::string& mdb_instance, const std::string& last_mdb_instance)
{
    if(!odb_->is_open())
    {
        if(!odb_->open())
        {
            LOG(ERROR)<<"odb open fail"<<std::endl;
            return false;
        }
    }
    //if(historydb_)
    //{
        //if(!historydb_->is_open())
        //{
            //if(!historydb_->open())
            //{
                //LOG(ERROR)<<"B5MHistoryDBHelper open fail"<<std::endl;
                //return false;
            //}
        //}
    //}
    //else
    //{
        //if(log_server_cfg_ == NULL)
            //return false;
        //// use logserver instead of local history 
        //if(!LogServerClient::Init(*log_server_cfg_))
        //{
            //LOG(ERROR) << "Log Server Init failed." << std::endl;
            //return false;
        //}
        //if(!LogServerClient::Test())
        //{
            //LOG(ERROR) << "log server test failed" << std::endl;
            //return false;
        //}
    //}

    boost::unordered_set<uint128_t> pid_changed_oid;

    std::string scd_path = B5MHelper::GetRawPath(mdb_instance);
    std::string output_dir = B5MHelper::GetB5moPath(mdb_instance);
    typedef izenelib::util::UString UString;
    namespace bfs = boost::filesystem;
    B5MHelper::PrepareEmptyDir(output_dir);

    std::string match_file = mdb_instance+"/cmatch";
    if(bfs::exists(match_file))
    {
        LOG(INFO)<<"odb loading match file "<<match_file<<std::endl;
        odb_->load_text(match_file, pid_changed_oid);
        if(!odb_->flush())
        {
            LOG(ERROR)<<"odb flush error"<<std::endl;
        }
    }
    ScdWriter b5mo_i(output_dir, INSERT_SCD);
    ScdWriter b5mo_u(output_dir, UPDATE_SCD);
    ScdWriter b5mo_d(output_dir, DELETE_SCD);

    std::vector<std::string> scd_list;
    B5MHelper::GetScdList(scd_path, scd_list);


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
                    soldpid = p->second;
                }
            }
            Document::doc_prop_value_strtype pdocid;
            doc.getString("DOCID", pdocid);
            std::string sdocid = propstr_to_str(pdocid);
            //doc.property("lastmodified") = UString(ts_, UString::UTF_8);
            std::string spid;
            if(!odb_->get(sdocid, spid))
            {
                spid = sdocid;
                odb_->insert(sdocid, spid);
            }
            doc.property("uuid") = str_to_propstr(spid);
            //std::string olduuid;
            //doc.getString("olduuid", olduuid);

            uint128_t oid = B5MHelper::StringToUint128(sdocid);
            if(!pid_changed_oid.empty())
            {
                pid_changed_oid.erase(oid);
            }

            switch (scd_type)
            {
            case INSERT_SCD:
                b5mo_i.Append(doc);
                break;

            case UPDATE_SCD:
            case RTYPE_SCD:
                b5mo_u.Append(doc);
                break;

            case DELETE_SCD:
                b5mo_d.Append(doc);
                break;
            }
        }
    }
    if(!pid_changed_oid.empty())
    {
        if(last_mdb_instance.empty())
        {
            LOG(ERROR)<<"pid changed oid not empty but last mdb instance not specify"<<std::endl;
        }
        else
        {
            std::vector<std::string> last_b5mo_scd_list;
            std::string last_b5mo_mirror_scd_path = B5MHelper::GetB5moMirrorPath(last_mdb_instance);
            B5MHelper::GetScdList(last_b5mo_mirror_scd_path, last_b5mo_scd_list);
            if(last_b5mo_scd_list.size()!=1)
            {
                LOG(ERROR)<<"last b5mo scd size should be 1"<<std::endl;
            }
            else
            {
                std::string scd_file = last_b5mo_scd_list[0];
                ScdParser parser(izenelib::util::UString::UTF_8);
                parser.load(scd_file);
                uint32_t n=0;
                for( ScdParser::iterator doc_iter = parser.begin();
                  doc_iter!= parser.end(); ++doc_iter, ++n)
                {
                    if(pid_changed_oid.empty()) break;
                    if(n%10000==0)
                    {
                        LOG(INFO)<<"Last b5mo mirror find Documents "<<n<<std::endl;
                    }
                    std::string soldpid;
                    Document doc;
                    SCDDoc& scddoc = *(*doc_iter);
                    SCDDoc::iterator p = scddoc.begin();
                    for(; p!=scddoc.end(); ++p)
                    {
                        const std::string& property_name = p->first;
                        doc.property(property_name) = p->second;
                    }
                    Document::doc_prop_value_strtype pdocid;
                    std::string sdocid;
                    doc.getString("DOCID", pdocid);
                    sdocid = propstr_to_str(pdocid);
                    uint128_t oid = B5MHelper::StringToUint128(sdocid);
                    if(pid_changed_oid.find(oid)!=pid_changed_oid.end())
                    {
                        std::string spid;
                        if(odb_->get(sdocid, spid))
                        {
                            doc.property("uuid") = spid;
                            b5mo_u.Append(doc);
                        }
                        pid_changed_oid.erase(oid);
                    }
                }

            }
        }
    }
    if(!odb_->flush())
    {
        LOG(WARNING)<<"odb flush error"<<std::endl;
    }
    //if(historydb_)
    //{
        //if(!historydb_->flush())
        //{
            //LOG(WARNING)<<"B5MHistoryDBHelper flush error"<<std::endl;
        //}
    //}
    //else
    //{
        //LogServerClient::Flush();
    //}
 
    b5mo_i.Close();
    b5mo_u.Close();
    b5mo_d.Close();
    return true;
}

