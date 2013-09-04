#include "b5mc_scd_generator.h"
#include "b5m_types.h"
#include "b5m_mode.h"
#include "b5m_helper.h"
#include "scd_doc_processor.h"
#include <common/ScdParser.h>
#include <common/ScdWriter.h>
#include <common/Utilities.h>
#include <product-manager/product_price.h>
#include <product-manager/uuid_generator.h>
#include <am/sequence_file/ssfr.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>

using namespace sf1r;
//#define B5MC_DEBUG


B5mcScdGenerator::B5mcScdGenerator(int mode, ProductMatcher* matcher)
:mode_(mode), matcher_(matcher)
,new_count_(0), pid_changed_count_(0), pid_not_changed_count_(0)
{
}

bool B5mcScdGenerator::Generate(const std::string& scd_path, const std::string& mdb_instance, const std::string& last_mdb_instance, int thread_num)
{
    std::string cdb_path = mdb_instance+"/cdb";
    cdb_.reset(new CommentDb(cdb_path));
    std::string odb_path = mdb_instance+"/odb";
    boost::shared_ptr<OfferDb> this_odb(new OfferDb(odb_path));
    boost::shared_ptr<OfferDb> last_odb;
    if(!last_mdb_instance.empty())
    {
        std::string last_odb_path = last_mdb_instance+"/odb";
        last_odb.reset(new OfferDb(last_odb_path));
    }
    odb_.reset(new OfferDbRecorder(this_odb.get(), last_odb.get()));
    if(!cdb_->is_open())
    {
        if(!cdb_->open())
        {
            LOG(ERROR)<<"cdb open fail"<<std::endl;
            return false;
        }
    }
    LOG(INFO)<<"cdb count "<<cdb_->Count()<<std::endl;
    if(!odb_->is_open())
    {
        if(!odb_->open())
        {
            LOG(ERROR)<<"odb open fail"<<std::endl;
            return false;
        }
    } 
    std::string output_dir = B5MHelper::GetB5mcPath(mdb_instance);
    B5MHelper::PrepareEmptyDir(output_dir);
    ScdDocProcessor::ProcessorType p = boost::bind(&B5mcScdGenerator::Process, this, _1);
    ScdDocProcessor sd_processor(p, thread_num);
    sd_processor.AddInput(scd_path);
    sd_processor.SetOutput(output_dir);
    sd_processor.Process();
    cdb_->flush();
    LOG(INFO)<<"new cid "<<new_count_<<", pid_changed "<<pid_changed_count_<<", pid_not_changed "<<pid_not_changed_count_<<std::endl;
    return true;
}

void B5mcScdGenerator::Process(ScdDocument& doc)
{
    static const std::string oid_property_name = "ProdDocid";
    doc.eraseProperty(B5MHelper::GetBrandPropertyName());
    std::string spid;
    doc.getString("uuid", spid);
    if(!spid.empty())
    {
        new_count_++;
        return;
    }
    doc.eraseProperty("uuid");
    std::string scid;
    doc.getString("DOCID", scid);
    if(scid.empty()) 
    {
        doc.type = NOT_SCD;
        return;
    }
    uint128_t cid = B5MHelper::StringToUint128(scid);
    bool has_oid = false;
    std::string soid;
    doc.getString(oid_property_name, soid);
    if(!soid.empty()) has_oid = true;

    bool is_new_cid = true;
    {
        boost::unique_lock<boost::mutex> lock(mutex_);
        if(cdb_->Get(cid)) 
        {
            is_new_cid = false;
        }
        else {
            cdb_->Insert(cid);
            new_count_++;
        }
    }
    //if(mode_==B5MMode::INC&&cdb_->Count()==0)
    //{
        //is_new_cid = false;
    //}
    //else{
    //}
    //{
        //bool test_is_new_cid = true;
        //if(uset.find(cid)!=uset.end())
        //{
            //test_is_new_cid = false;
        //}
        //else
        //{
            //uset.insert(cid);
        //}
        //if(is_new_cid!=test_is_new_cid)
        //{
            //LOG(ERROR)<<"filter error on "<<scid<<std::endl;
        //}
    //}
    bool pid_changed = false;
    if(has_oid)
    {
        odb_->get(soid, spid, pid_changed);
    }
    if(pid_changed)
    {
        pid_changed_count_++;
#ifdef B5MC_DEBUG
        std::string last_spid;
        odb_->get_last(soid, last_spid);
        LOG(INFO)<<soid<<" changed from "<<last_spid<<" to "<<spid<<std::endl;
#endif
    }
    else
    {
        if(!spid.empty())
        {
            pid_not_changed_count_++;
        }
    }
    if(!spid.empty())
    {
        doc.property("uuid") = str_to_propstr(spid);
    }
    //std::cerr<<is_new_cid<<","<<pid_changed<<","<<spid<<std::endl;
    bool need_process = is_new_cid || pid_changed;
    if(!need_process) 
    {
        doc.type = NOT_SCD;
        return;
    }
    ProcessFurther_(doc);
    spid.clear();
    doc.getString("uuid", spid);
    if(spid.empty()) doc.type = NOT_SCD;
}

void B5mcScdGenerator::ProcessFurther_(ScdDocument& doc)
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
            //book_doc.property("Category") = str_to_propstr(B5MHelper::BookCategoryName());
            book_doc.property("Attribute") = doc.property("Attribute");
            ProductMatcher::ProcessBookStatic(book_doc, product);
        }
        else
        {
            Document::doc_prop_value_strtype title;
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
    //if(!spid.empty() && bdb_!=NULL)
    //{
        //uint128_t pid = B5MHelper::StringToUint128(spid);
        //Document::doc_prop_value_strtype brand;
        //bdb_->get(pid, brand);
        //if(brand.length()>0)
        //{
            //doc.property(B5MHelper::GetBrandPropertyName()) = brand;
        //}
    //}
    if(!spid.empty())
    {
        doc.property("uuid") = str_to_propstr(spid, UString::UTF_8);
    }
}

ProductMatcher* B5mcScdGenerator::GetMatcher_()
{
    if(matcher_==NULL) return NULL;
    //if(!matcher_->IsOpen())
    //{
        //matcher_->Open();
        //matcher_->SetUsePriceSim(false);
    //}
    return matcher_;
}
