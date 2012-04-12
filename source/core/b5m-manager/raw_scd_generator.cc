#include "raw_scd_generator.h"
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

RawScdGenerator::RawScdGenerator(OfferDb* odb)
:odb_(odb)
{
}


bool RawScdGenerator::Generate(const std::string& scd_path, const std::string& output_dir)
{
    if(!odb_->is_open())
    {
        if(!odb_->open())
        {
            LOG(ERROR)<<"odb open fail"<<std::endl;
            return false;
        }
    }
    typedef izenelib::util::UString UString;
    namespace bfs = boost::filesystem;
    bfs::create_directories(output_dir);

    std::vector<std::string> scd_list;
    B5MHelper::GetScdList(scd_path, scd_list);
    if(scd_list.empty()) return false;

    //ScdWriterController writer(output_dir);
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
        for( ScdParser::iterator doc_iter = parser.begin(B5MHelper::B5MO_PROPERTY_LIST.value);
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
            std::string sdocid;
            doc.getString("DOCID", sdocid);
            if(sdocid.empty()) continue;
            int doc_type = NOT_SCD;
            if(scd_type==INSERT_SCD)
            {
                if(odb_->has_key(sdocid)) continue;
                doc_type = INSERT_SCD;
            }
            else if(scd_type==UPDATE_SCD)
            {
                if(odb_->has_key(sdocid))
                {
                    doc_type = UPDATE_SCD;
                }
                else
                {
                    doc_type = INSERT_SCD;
                }
            }
            else if(scd_type==DELETE_SCD)
            {
                if(!odb_->has_key(sdocid)) continue;
                doc_type = DELETE_SCD;
            }
            if(doc_type==INSERT_SCD)
            {
                b5mo_i.Append(doc);
            }
            else if(doc_type==UPDATE_SCD)
            {
                b5mo_u.Append(doc);
            }
            else if(doc_type==DELETE_SCD)
            {
                b5mo_d.Append(doc);
            }
        }
    }
    b5mo_i.Close();
    b5mo_u.Close();
    b5mo_d.Close();
    return true;
}

