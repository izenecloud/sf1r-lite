#include "b5mp_uue_processor.h"
#include "b5m_types.h"
#include "b5m_helper.h"
#include <common/ScdParser.h>
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

B5MPUueProcessor::B5MPUueProcessor(const std::string& b5mo_scd, const std::string& b5mp_scd, OfferDb* odb, ProductDb* product_db, const std::string& work_path, bool reindex, bool fast_mode)
:b5mo_scd_(b5mo_scd), b5mp_scd_(b5mp_scd), odb_(odb), product_db_(product_db), work_path_(work_path), reindex_(reindex), fast_mode_(fast_mode)
{
}

void B5MPUueProcessor::Process(const UueItem& item)
{
    o2p_map_[item.docid] = item.from_to;
}

void B5MPUueProcessor::Finish()
{
    if(!odb_->is_open())
    {
        if(!odb_->open())
        {
            LOG(ERROR)<<"odb open fail"<<std::endl;
            return;
        }
    }
    std::vector<std::string> scd_list;
    B5MHelper::GetScdList(b5mo_scd_, scd_list);
    if(scd_list.empty()) return;

    std::string working_dir = work_path_;
    if(working_dir.empty())
    {
        working_dir = "./b5mp_generator_working";
    }
    boost::filesystem::remove_all(working_dir);
    boost::filesystem::create_directories(working_dir);
    std::string writer_file = working_dir+"/docs";
    izenelib::am::ssf::Writer<> writer(writer_file);
    writer.Open();

    uint64_t FLAG_APPEND = 1;
    uint64_t FLAG_REMOVE = 2;

    for(uint32_t i=0;i<scd_list.size();i++)
    {
        std::string scd_file = scd_list[i];
        int scd_type = ScdParser::checkSCDType(scd_file);
        LOG(INFO)<<"Processing "<<scd_file<<std::endl;
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
            SCDDoc::iterator p = scddoc.begin();
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
                doc.property(property_name) = p->second;
            }
            //if(exclude_)
            //{
                //std::string scategory;
                //doc.property("Category").get<UString>().convertString(scategory, UString::UTF_8);
                //bool find_match = false;
                //for(uint32_t i=0;i<category_regex_.size();i++)
                //{
                    //if(boost::regex_match(scategory, category_regex_[i]))
                    //{
                        //find_match = true;
                        //break;
                    //}
                //}
                //if(!find_match) continue;
            //}
            std::string sdocid;
            UString udocid;
            doc.getProperty("DOCID", udocid);
            udocid.convertString(sdocid, UString::UTF_8);
            std::string old_source;
            std::string new_source;
            UueFromTo from_to;
            O2PMap::iterator it = o2p_map_.find(sdocid);
            if(it==o2p_map_.end())
            {
                continue;
                //this is a not valid doc
            }
            else
            {
                from_to = it->second;
            }
            if(from_to.from.length()>0)
            {
                Document rdoc;
                rdoc.property("DOCID") = udocid;
                uint128_t from_id = B5MHelper::StringToUint128(from_to.from);
                rdoc.property("flag") = FLAG_REMOVE;
                OfferDb::ValueType ovalue;
                if(odb_->get(sdocid, ovalue))
                {
                    old_source = ovalue.source;
                    new_source=old_source;
                    if(!old_source.empty()) rdoc.property("Source") = UString(old_source, UString::UTF_8);
                }
                //if(from_to.from!=from_to.to)
                //{
                //}
                writer.Append(from_id, rdoc);
            }
            if(from_to.to.length()>0)
            {
                uint128_t to_id = B5MHelper::StringToUint128(from_to.to);
                doc.property("flag") = FLAG_APPEND;
                std::string doc_source;
                if(doc.getString("Source", doc_source))
                {
                    new_source = doc_source;
                }
                else if(new_source.length()>0)
                {
                    doc.property("Source") = UString(new_source, UString::UTF_8);
                }
                writer.Append(to_id, doc);
            }
            //TODO update odb here;
            bool need_update_odb = false;
            if(from_to.to!=from_to.from)
            {
                need_update_odb = true;
            }
            if(old_source!=new_source)
            {
                need_update_odb = true;
            }
            if(need_update_odb)
            {
                OfferDb::ValueType ovalue;
                ovalue.pid = from_to.to;
                ovalue.source = new_source;
                odb_->update(sdocid, ovalue);
                //LOG(INFO)<<"odb update "<<sdocid<<","<<ovalue.pid<<"-"<<ovalue.source<<std::endl;
            }
        }
    }
    writer.Close();
    izenelib::am::ssf::Sorter<uint32_t, uint128_t>::Sort(writer_file);
    izenelib::am::ssf::Joiner<uint32_t, uint128_t, Document> joiner(writer_file);
    joiner.Open();
    uint128_t key;
    std::vector<Document> docs;
    boost::filesystem::create_directories(b5mp_scd_);
    //ScdWriterController b5mp_writer(b5mp_scd_);
    ScdWriter b5mp_i(b5mp_scd_, INSERT_SCD);
    ScdWriter b5mp_u(b5mp_scd_, UPDATE_SCD);
    ScdWriter b5mp_d(b5mp_scd_, DELETE_SCD);
    uint64_t n = 0;
    while(joiner.Next(key, docs))
    {
        ++n;
        if(n%100000==0)
        {
            LOG(INFO)<<"Process "<<n<<" docs"<<std::endl;
        }
        std::string pid = B5MHelper::Uint128ToString(key);
        UString upid(pid, UString::UTF_8);
        ProductProperty product;
        product_db_->get(pid, product);
        LOG(INFO)<<"pid "<<pid<<" before : "<<product.ToString()<<std::endl;
        ProductProperty new_product(product);
        for(uint32_t i=0;i<docs.size();i++)
        {
            uint64_t flag = 0;
            docs[i].getProperty("flag", flag);
            std::string sprice;
            docs[i].getString("Price", sprice);
            ProductProperty pp(docs[i]);
            if(flag==FLAG_APPEND)
            {
                //LOG(INFO)<<"+= on pid "<<pid<<std::endl;
                //LOG(INFO)<<"before += "<<new_product.ToString()<<std::endl;
                //LOG(INFO)<<"price+= "<<sprice<<std::endl;
                //LOG(INFO)<<"price+= "<<pp.price.ToString()<<std::endl;
                LOG(INFO)<<"+= "<<pp.ToString()<<std::endl;
                new_product += pp;
                //LOG(INFO)<<"after += "<<new_product.ToString()<<std::endl;
            }
            else
            {
                //LOG(INFO)<<"-= on pid "<<pid<<std::endl;
                //LOG(INFO)<<"before -= "<<new_product.ToString()<<std::endl;
                LOG(INFO)<<"-= "<<pp.ToString()<<std::endl;
                new_product -= pp;
                //LOG(INFO)<<"after -= "<<new_product.ToString()<<std::endl;
            }
        }
        LOG(INFO)<<"pid "<<pid<<" after : "<<new_product.ToString()<<std::endl;
        int op_type = -1;
        if(product.itemcount==0 && new_product.itemcount>0)
        {
            op_type = INSERT_SCD;
        }
        else if(product.itemcount>0 && new_product.itemcount>0)
        {
            op_type = UPDATE_SCD;
        }
        else if(product.itemcount>0 && new_product.itemcount==0)
        {
            op_type = DELETE_SCD;
        }
        else
        {
            //unexpected error
            LOG(ERROR)<<"product value error , itemcount now "<<new_product.itemcount<<" , pid "<<pid<<std::endl;
        }
        if(op_type==INSERT_SCD)
        {
            Document doc;
            bool find = false;
            for(uint32_t i=0;i<docs.size();i++)
            {
                UString udocid;
                docs[i].getProperty("DOCID", udocid);
                if(udocid == upid)
                {
                    doc.copyPropertiesFromDocument(docs[i]);
                    find = true;
                    break;
                }
            }
            if(!find)
            {
                doc.copyPropertiesFromDocument(docs[0]);
            }
            doc.property("DOCID") = upid;
            UString new_uprice = new_product.price.ToUString();
            UString new_usource = new_product.GetSourceUString();
            doc.property("Price") = new_uprice;
            doc.property("Source") = new_usource;
            doc.property("itemcount") = (uint64_t)new_product.itemcount;
            doc.eraseProperty("flag");
            doc.eraseProperty("uuid");
            b5mp_i.Append(doc);
            product_db_->insert(pid, new_product);
            
        }
        else if(op_type==UPDATE_SCD)
        {
            Document doc;
            doc.property("DOCID") = upid;
            UString uprice = product.price.ToUString();
            UString new_uprice = new_product.price.ToUString();
            UString usource = product.GetSourceUString();
            UString new_usource = new_product.GetSourceUString();
            bool need_scd_update = false;
            bool need_db_update = false;
            if(uprice!=new_uprice)
            {
                doc.property("Price") = new_uprice;
                need_scd_update = true;
                need_db_update = true;
            }
            if(product.itemcount!=new_product.itemcount)
            {
                doc.property("itemcount") = (uint64_t)new_product.itemcount;
                need_scd_update = true;
                need_db_update = true;
            }
            if(usource!=new_usource)
            {
                doc.property("Source") = new_usource;
                need_scd_update = true;
            }
            if(product.source!=new_product.source)
            {
                need_db_update = true;
            }
            if(new_product.itemcount==1 && product.itemcount==1)
            {
                for(uint32_t i=0;i<docs.size();i++)
                {
                    uint64_t flag = 0;
                    docs[i].getProperty("flag", flag);
                    if(flag==FLAG_APPEND)
                    {
                        for(Document::property_const_iterator pit = docs[i].propertyBegin(); pit!=docs[i].propertyEnd(); ++pit)
                        {
                            const std::string& pname = pit->first;
                            if(pname=="Title" || pname=="Url" || pname=="Picture" || pname=="Content")
                            {
                                doc.property(pname) = pit->second;
                                need_scd_update = true;
                            }
                        }
                    }
                }
            }
            if(need_scd_update)
            {
                b5mp_u.Append(doc);
            }
            if(need_db_update)
            {
                product_db_->update(pid, new_product);
            }
            
            
        }
        else if(op_type==DELETE_SCD)
        {
            Document doc;
            doc.property("DOCID") = upid;
            b5mp_d.Append(doc);
            //b5mp_writer.Write(doc, DELETE_SCD);
            product_db_->del(pid);
        }
    }
    b5mp_i.Close();
    b5mp_u.Close();
    b5mp_d.Close();
    boost::filesystem::remove_all(working_dir);
    LOG(INFO)<<"flushing odb.."<<std::endl;
    odb_->flush();
    LOG(INFO)<<"flush odb complete"<<std::endl;
}

