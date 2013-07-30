#include "b5mc_scd_generator.h"
#include "comment_db.h"
#include "offer_db.h"
#include "offer_db_recorder.h"
#include "b5m_types.h"
#include "b5m_mode.h"
#include "b5m_helper.h"
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
{
}

bool B5mcScdGenerator::Generate(const std::string& scd_path, const std::string& mdb_instance, const std::string& last_mdb_instance)
{
    std::string cdb_path = mdb_instance+"/cdb";
    boost::shared_ptr<CommentDb> cdb(new CommentDb(cdb_path));
    std::string odb_path = mdb_instance+"/odb";
    boost::shared_ptr<OfferDb> this_odb(new OfferDb(odb_path));
    boost::shared_ptr<OfferDb> last_odb;
    if(!last_mdb_instance.empty())
    {
        std::string last_odb_path = last_mdb_instance+"/odb";
        last_odb.reset(new OfferDb(last_odb_path));
    }
    boost::shared_ptr<OfferDbRecorder> odb(new OfferDbRecorder(this_odb.get(), last_odb.get()));
    if(!cdb->is_open())
    {
        if(!cdb->open())
        {
            LOG(ERROR)<<"cdb open fail"<<std::endl;
            return false;
        }
    }
    LOG(INFO)<<"cdb count "<<cdb->Count()<<std::endl;
    if(!odb->is_open())
    {
        if(!odb->open())
        {
            LOG(ERROR)<<"odb open fail"<<std::endl;
            return false;
        }
    } 
    //if(bdb_!=NULL)
    //{
        //if(!bdb_->is_open())
        //{
            //if(!bdb_->open())
            //{
                //LOG(ERROR)<<"bdb open error"<<std::endl;
                //return false;
            //}
        //}
    //}
    std::vector<std::string> scd_list;
    B5MHelper::GetScdList(scd_path, scd_list);
    if(scd_list.empty())
    {
        LOG(WARNING)<<"scd path empty"<<std::endl;
        return true;
    }
    static const std::string oid_property_name = "ProdDocid";
    std::string output_dir = B5MHelper::GetB5mcPath(mdb_instance);
    B5MHelper::PrepareEmptyDir(output_dir);
    ScdWriter b5mc_u(output_dir, UPDATE_SCD);
    std::size_t new_count = 0;
    std::size_t pid_changed_count = 0;
    std::size_t pid_not_changed_count = 0;
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
                LOG(INFO)<<"Find Documents "<<n<<","<<new_count<<","<<pid_changed_count<<","<<pid_not_changed_count<<std::endl;
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
            if(cdb->Get(cid)) 
            {
                is_new_cid = false;
            }
            else {
                cdb->Insert(cid);
                new_count++;
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
            std::string spid;
            bool pid_changed = false;
            if(has_oid)
            {
                odb->get(soid, spid, pid_changed);
            }
            if(pid_changed)
            {
                pid_changed_count++;
#ifdef B5MC_DEBUG
                std::string last_spid;
                odb->get_last(soid, last_spid);
                LOG(INFO)<<soid<<" changed from "<<last_spid<<" to "<<spid<<std::endl;
#endif
            }
            else
            {
                if(!spid.empty())
                {
                    pid_not_changed_count++;
                }
            }
            if(!spid.empty())
            {
                doc.property("uuid") = str_to_propstr(spid);
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
    cdb->flush();
    LOG(INFO)<<"new cid "<<new_count<<", pid_changed "<<pid_changed_count<<", pid_not_changed "<<pid_not_changed_count<<std::endl;
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
