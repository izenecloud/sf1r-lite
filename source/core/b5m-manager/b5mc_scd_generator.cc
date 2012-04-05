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

B5MCScdGenerator::B5MCScdGenerator(OfferDb* odb): odb_(odb)
{
}


bool B5MCScdGenerator::Generate(const std::string& scd_path, const std::string& output_dir)
{
    if(odb_->is_open())
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

    ScdWriterController writer(output_dir);

    for(uint32_t i=0;i<scd_list.size();i++)
    {
        std::string scd_file = scd_list[i];
        LOG(INFO)<<"Processing "<<scd_file<<std::endl;
        int scd_type = ScdParser::checkSCDType(scd_file);
        if(scd_type!=INSERT_SCD) continue;
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(scd_file);
        uint32_t n=0;
        for( ScdParser::iterator doc_iter = parser.begin(B5MHelper::B5MC_PROPERTY_LIST);
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
            UString oid;
            if(!doc.getProperty("ProdDocid", oid)) continue;

            std::string soid;
            oid.convertString(soid, UString::UTF_8);
            //std::string spid = sdocid;
            //std::string spid;
            OfferDb::ValueType ovalue;
            if(!odb_->get(soid, ovalue)) continue;
            doc.property("uuid") = UString(ovalue.pid, UString::UTF_8);
            writer.Write(doc, scd_type);
        }
    }
    writer.Flush();
    return true;
}

