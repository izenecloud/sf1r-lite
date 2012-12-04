#include "b5mc_scd_generator.h"
#include "b5m_types.h"
#include "b5m_mode.h"
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


B5mcScdGenerator::B5mcScdGenerator(CommentDb* cdb, OfferDbRecorder* odb, BrandDb* bdb, ProductMatcher* matcher, int mode)
:cdb_(cdb), odb_(odb), bdb_(bdb), matcher_(matcher), mode_(mode)
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
    if(!cdb_->is_open())
    {
        if(!cdb_->open())
        {
            LOG(ERROR)<<"cdb open fail"<<std::endl;
            return false;
        }
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
    ScdWriter b5mc_u(output_dir, UPDATE_SCD);
    //ScdWriter b5mc_d(output_dir, DELETE_SCD);
    for(uint32_t i=0;i<scd_list.size();i++)
    {
        std::string scd_file = scd_list[i];
        int type = ScdParser::checkSCDType(scd_file);
        if(type==DELETE_SCD) continue;
        LOG(INFO)<<"Processing "<<scd_file<<std::endl;
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
            Document doc;
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
                doc.property(property_name) = p->second;
            }
            doc.eraseProperty("uuid");
            doc.eraseProperty(B5MHelper::GetBrandPropertyName());
            std::string scid;
            doc.getString("DOCID", scid);
            if(scid.empty()) continue;
            uint128_t cid = B5MHelper::StringToUint128(scid);
            bool has_oid = false;
            std::string soid;
            doc.getString(oid_property_name, soid);
            if(!soid.empty()) has_oid = true;

            bool is_new_cid = true;
            if(mode_==B5MMode::INC&&cdb_->Count()==0)
            {
                is_new_cid = false;
            }
            else{
                if(cdb_->Get(cid)) 
                {
                    is_new_cid = false;
                }
                else {
                    cdb_->Insert(cid);
                }
            }
            std::string spid;
            bool pid_changed = false;
            if(has_oid)
            {
                odb_->get(soid, spid, pid_changed);
            }
            if(!spid.empty())
            {
                doc.property("uuid") = UString(spid, UString::UTF_8);
            }
            //std::cerr<<is_new_cid<<","<<pid_changed<<","<<spid<<std::endl;
            bool need_process = is_new_cid || pid_changed;
            if(!need_process) continue;
            ProcessFurther_(doc);
            spid.clear();
            doc.getString("uuid", spid);
            if(spid.empty()) continue;
            b5mc_u.Append(doc);
        } 
    }
    b5mc_u.Close();
    cdb_->flush();
    return true;
}

void B5mcScdGenerator::ProcessFurther_(Document& doc)
{
    static const std::string title_property_name = "ProdName";
    std::string spid;
    doc.getString("uuid", spid);
    if(spid.empty())
    {
        ProductMatcher::Product product;
        std::string isbn;
        if(ProductMatcher::GetIsbnAttribute(doc, isbn))
        {
            Document book_doc;
            book_doc.property("Category") = UString(B5MHelper::BookCategoryName(), UString::UTF_8);
            book_doc.property("Attribute") = doc.property("Attribute");
            ProductMatcher::ProcessBook(book_doc, product);
        }
        else
        {
            UString title;
            doc.getProperty(title_property_name, title);
            if(!title.empty())
            {
                Document matcher_doc;
                matcher_doc.property("Title") = title;
                ProductMatcher* matcher = GetMatcher_();
                if(matcher!=NULL)
                {
                    matcher->Process(matcher_doc, product);
                }
            }
        }
        if(!product.spid.empty())
        {
            spid = product.spid;
        }
    }
    if(!spid.empty() && bdb_!=NULL)
    {
        uint128_t pid = B5MHelper::StringToUint128(spid);
        izenelib::util::UString brand;
        bdb_->get(pid, brand);
        if(brand.length()>0)
        {
            doc.property(B5MHelper::GetBrandPropertyName()) = brand;
        }
    }
    if(!spid.empty())
    {
        doc.property("uuid") = UString(spid, UString::UTF_8);
    }
}

ProductMatcher* B5mcScdGenerator::GetMatcher_()
{
    if(matcher_==NULL) return NULL;
    if(!matcher_->IsOpen())
    {
        matcher_->Open();
        matcher_->SetUsePriceSim(false);
    }
    return matcher_;
}
