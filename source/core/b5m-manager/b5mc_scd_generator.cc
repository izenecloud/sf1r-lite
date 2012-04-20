#include "b5mc_scd_generator.h"
#include "b5m_types.h"
#include "b5m_helper.h"
#include <common/ScdParser.h>
#include <common/ScdWriter.h>
#include <common/ScdWriterController.h>
#include <common/Utilities.h>
#include <product-manager/product_price.h>
#include <product-manager/uuid_generator.h>
#include <am/sequence_file/ssfr.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>

using namespace sf1r;

//B5MCScdGenerator::B5MCScdGenerator(const std::string& scd_path, OfferDb* odb, CommentDb* cdb, const std::string& output_dir, const std::string& work_path)
//:scd_path_(scd_path), odb_(odb), cdb_(cdb), output_dir_(output_dir), work_path_(work_path)
//{
//}

B5MCScdGenerator::B5MCScdGenerator(const std::string& new_scd, const std::string& old_scd, OfferDb* odb, const std::string& output_dir)
:new_scd_(new_scd), old_scd_(old_scd), odb_(odb), output_dir_(output_dir)
{
}

//B5MCScdGenerator::B5MCScdGenerator(OfferDb* odb, const std::string& mdb)
//:odb_(odb), mdb_(mdb)
//{
//}

void B5MCScdGenerator::Process(const UueItem& item)
{
    if(item.from_to.from.length()>0 && item.from_to.from!=item.from_to.to)
    {
        modified_.insert(std::make_pair(item.docid, item.from_to.to));
        LOG(INFO)<<"uue modified "<<item.docid<<" from "<<item.from_to.from<<" to "<<item.from_to.to<<std::endl; 
    }
}

void B5MCScdGenerator::Finish()
{
    typedef izenelib::util::UString UString;
    namespace bfs = boost::filesystem;
    std::string oid_property_name = "ProdDocid";
    bfs::create_directories(output_dir_);
    ScdWriter b5mc_i(output_dir_, INSERT_SCD);
    ScdWriter b5mc_u(output_dir_, UPDATE_SCD);
    //ScdWriter b5mc_d(output_dir_, DELETE_SCD);
    std::vector<std::string> old_scd_list;
    if(!old_scd_.empty())
    {
        B5MHelper::GetScdList(old_scd_, old_scd_list);
    }
    if(old_scd_list.size()==1)
    {
        std::string scd_file = old_scd_list[0];
        int scd_type = ScdParser::checkSCDType(scd_file);
        LOG(INFO)<<"Processing old "<<scd_file<<std::endl;
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(scd_file);
        uint32_t n=0;
        for( ScdParser::iterator doc_iter = parser.begin(B5MHelper::B5MC_PROPERTY_LIST.value);
          doc_iter!= parser.end(); ++doc_iter, ++n)
        {
            if(n%10000==0)
            {
                LOG(INFO)<<"Find Documents "<<n<<std::endl;
            }
            SCDDoc& scddoc = *(*doc_iter);
            std::vector<std::pair<izenelib::util::UString, izenelib::util::UString> >::iterator p;
            std::string sdocid;
            std::string soid;
            std::string spid;
            for(p=scddoc.begin(); p!=scddoc.end(); ++p)
            {
                std::string property_name;
                p->first.convertString(property_name, izenelib::util::UString::UTF_8);
                if(property_name=="DOCID")
                {
                    p->second.convertString(sdocid, izenelib::util::UString::UTF_8);
                }
                else if(property_name==oid_property_name)
                {
                    p->second.convertString(soid, izenelib::util::UString::UTF_8);
                }
                else if(property_name=="uuid")
                {
                    p->second.convertString(spid, izenelib::util::UString::UTF_8);
                }
            }
            if(sdocid.empty()||soid.empty()) continue;
            //LOG(INFO)<<"find oid in old "<<soid<<std::endl;
            ModifyMap::const_iterator it = modified_.find(soid);
            if(it!=modified_.end())
            {
                Document doc;
                doc.property("DOCID") = izenelib::util::UString(sdocid, izenelib::util::UString::UTF_8);
                //LOG(INFO)<<"modified "<<sdocid<<std::endl;
                //keep it even if spid is empty
                doc.property("uuid") = UString(it->second, UString::UTF_8);
                b5mc_u.Append(doc);
                //if(it->second.empty())
                //{
                    //b5mc_d.Append(doc);
                //}
                //else
                //{
                //}
            }

        }

    }
    modified_.clear();
    std::vector<std::string> scd_list;
    B5MHelper::GetScdList(new_scd_, scd_list);
    if(scd_list.empty())
    {
        LOG(INFO)<<"new scd empty"<<std::endl;
        return;
    }
    if(!odb_->is_open())
    {
        if(!odb_->open())
        {
            LOG(ERROR)<<"odb open fail"<<std::endl;
            return;
        }
    }

    for(uint32_t i=0;i<scd_list.size();i++)
    {
        std::string scd_file = scd_list[i];
        LOG(INFO)<<"Processing "<<scd_file<<std::endl;
        int scd_type = ScdParser::checkSCDType(scd_file);
        if(scd_type!=INSERT_SCD) continue;
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(scd_file);
        uint32_t n=0;
        for( ScdParser::iterator doc_iter = parser.begin(B5MHelper::B5MC_PROPERTY_LIST.value);
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

            std::string soid;
            if(!doc.getString(oid_property_name, soid)) continue;
            //std::string spid = sdocid;
            std::string spid;
            OfferDb::ValueType ovalue;
            if(odb_->get(soid, ovalue))
            {
                spid = ovalue.pid;
            }
            //keep the non-exist oid comment
            doc.property("uuid") = UString(spid, UString::UTF_8);
            b5mc_i.Append(doc);
        }
    }
    b5mc_i.Close();
    b5mc_u.Close();
    //b5mc_d.Close();
}

