#include "b5mc_scd_generator.h"
#include "b5m_types.h"
#include "b5m_helper.h"
#include <common/ScdParser.h>
#include <common/ScdWriter.h>
#include <common/ScdMerger.h>
#include <common/Utilities.h>
#include <product-manager/product_price.h>
#include <product-manager/uuid_generator.h>
#include <am/sequence_file/ssfr.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>

using namespace sf1r;


B5mcScdGenerator::B5mcScdGenerator(OfferDb* odb, BrandDb* bdb)
:odb_(odb), bdb_(bdb)
{
}

bool B5mcScdGenerator::Generate(const std::string& scd_path, const std::string& mdb_instance)
{
    std::vector<std::string> scd_list;
    B5MHelper::GetScdList(scd_path, scd_list);
    if(scd_list.empty())
    {
        LOG(WARNING)<<"scd path empty"<<std::endl;
        return true;
    }
    if(!odb_->is_open())
    {
        if(!odb_->open())
        {
            LOG(ERROR)<<"odb open fail"<<std::endl;
            return false;
        }
    }
    if(bdb_!=NULL)
    {
        if(!bdb_->is_open())
        {
            if(!bdb_->open())
            {
                LOG(ERROR)<<"bdb open error"<<std::endl;
                return false;
            }
        }
    }
    static const std::string oid_property_name = "ProdDocid";
    std::string output_dir = B5MHelper::GetB5mcPath(mdb_instance);
    B5MHelper::PrepareEmptyDir(output_dir);
    ScdWriter b5mc_i(output_dir, INSERT_SCD);
    ScdWriter b5mc_u(output_dir, UPDATE_SCD);
    ScdWriter b5mc_d(output_dir, DELETE_SCD);
    for(uint32_t i=0;i<scd_list.size();i++)
    {
        std::string scd_file = scd_list[i];
        LOG(INFO)<<"Processing "<<scd_file<<std::endl;
        int type = ScdParser::checkSCDType(scd_file);
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(scd_file);
        uint32_t n=0;
        for( ScdParser::iterator doc_iter = parser.begin();
          doc_iter!= parser.end(); ++doc_iter, ++n)
        {
            if(n%100000==0)
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
            }
            if(type!=DELETE_SCD)
            {
                std::string soid;
                if(doc.getString(oid_property_name, soid))
                {
                    std::string spid;
                    if(odb_->get(soid, spid))
                    {

                    }
                    izenelib::util::UString upid(spid, izenelib::util::UString::UTF_8);
                    doc.property("uuid") = upid;
                    if(upid.length()>0 && bdb_!=NULL)
                    {
                        uint128_t pid = B5MHelper::UStringToUint128(upid);
                        izenelib::util::UString brand;
                        bdb_->get(pid, brand);
                        if(brand.length()>0)
                        {
                            doc.property(B5MHelper::GetBrandPropertyName()) = brand;
                        }
                    }
                }
            }
            if(type==INSERT_SCD)
            {
                b5mc_i.Append(doc);
            }
            else if(type==UPDATE_SCD)
            {
                b5mc_u.Append(doc);
            }
            else if(type==DELETE_SCD)
            {
                b5mc_d.Append(doc);
            }
        }
    }
    b5mc_i.Close();
    b5mc_u.Close();
    b5mc_d.Close();
}

