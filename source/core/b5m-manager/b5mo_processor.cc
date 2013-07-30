#include "b5mo_processor.h"
#include "log_server_client.h"
#include "image_server_client.h"
#include "b5m_types.h"
#include "b5m_helper.h"
#include "b5m_mode.h"
#include "scd_doc_processor.h"
#include <common/Utilities.h>
#include <product-manager/product_price.h>
#include <product-manager/uuid_generator.h>
#include <configuration-manager/LogServerConnectionConfig.h>
#include <sf1r-net/RpcServerConnectionConfig.h>
#include <am/sequence_file/ssfr.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <glog/logging.h>

using namespace sf1r;

B5moProcessor::B5moProcessor(OfferDb* odb, ProductMatcher* matcher,
    int mode, 
    RpcServerConnectionConfig* img_server_config)
:odb_(odb), matcher_(matcher), sorter_(NULL), omapper_(NULL), mode_(mode), img_server_cfg_(img_server_config)
{
}

void B5moProcessor::LoadMobileSource(const std::string& file)
{
    std::ifstream ifs(file.c_str());
    std::string line;
    while( getline(ifs,line))
    {
        boost::algorithm::trim(line);
        mobile_source_.insert(line);
    }
    ifs.close();
}

void B5moProcessor::Process(Document& doc, SCD_TYPE& type)
{
    static const std::string tcp(B5MHelper::GetTargetCategoryPropertyName());
    //return;
    //reset type
    if(mode_!=B5MMode::INC)
    {
        if(type==DELETE_SCD)
        {
            type = NOT_SCD;
        }
        else
        {
            type = UPDATE_SCD;
        }
    }
    else
    {
        if(type==INSERT_SCD)
        {
            type = UPDATE_SCD;
        }
    }
    if(type==NOT_SCD) return;

    //format Price property
    Document::doc_prop_value_strtype uprice;
    if(doc.getProperty("Price", uprice))
    {
        ProductPrice pp;
        pp.Parse(propstr_to_ustr(uprice));
        doc.property("Price") = pp.ToPropString();
    }

    //set mobile tag
    Document::doc_prop_value_strtype usource;
    if(doc.getString("Source", usource))
    {
        std::string ssource = propstr_to_str(usource);
        if(mobile_source_.find(ssource)!=mobile_source_.end())
        {
            doc.property("mobile") = (int64_t)1;
        }
    }

    //set color tag
    if(img_server_cfg_)
    {
        Document::doc_prop_value_strtype propvalue;
        std::string img_file;
        doc.getString("Picture", propvalue);
        img_file = propstr_to_str(propvalue);
        if(!img_file.empty())
        {
            std::string color_str;
            ImageServerClient::GetImageColor(img_file, color_str);
            doc.property("Color") = str_to_propstr(color_str);
        }
    }

    //set pid(uuid) tag

    std::string sdocid;
    doc.getString("DOCID", sdocid);
    uint128_t oid = B5MHelper::StringToUint128(sdocid);
    sdocid = B5MHelper::Uint128ToString(oid); //to avoid DOCID error;
    std::string spid;
    std::string old_spid;
    if(type==RTYPE_SCD)
    {
        if(odb_->get(sdocid, spid)) 
        {
            doc.property("uuid") = str_to_propstr(spid);
            type = UPDATE_SCD;
        }
        else
        {
            LOG(ERROR)<<"rtype docid "<<sdocid<<" does not exist"<<std::endl;
            type = NOT_SCD;
        }
    }
    else if(type!=DELETE_SCD) //I or U
    {
        ProcessIU_(doc);
    }
    else
    {
        doc.clearExceptDOCID();
        odb_->get(sdocid, spid);
        old_spid = spid;
        if(spid.empty()) type=NOT_SCD;
        else
        {
            doc.property("uuid") = str_to_propstr(spid);
            if(sorter_!=NULL)
            {
                ScdDocument sdoc(doc, DELETE_SCD);
                sorter_->Append(sdoc, ts_);
            }
        }
    }
    if(!changed_match_.empty())
    {
        changed_match_.erase(oid);
    }
    //if(sorter_!=NULL)
    //{
        //if(type!=DELETE_SCD)
        //{
            //ScdDocument sdoc(doc, type);
            //sorter_->Append(sdoc, ts_);
            //if(old_spid!=spid&&!old_spid.empty())
            //{
                //ScdDocument old_doc;
                //old_doc.property("DOCID") = str_to_propstr(sdocid, UString::UTF_8);
                //old_doc.property("uuid") = str_to_propstr(old_spid, UString::UTF_8);
                //old_doc.type=DELETE_SCD;
                //sorter_->Append(old_doc, last_ts_);
            //}
        //}
        //else
        //{
            //ScdDocument sdoc(doc, DELETE_SCD);
            //sorter_->Append(sdoc, ts_);
        //}
    //}
}
void B5moProcessor::ProcessIU_(Document& doc, bool force_match)
{
    SCD_TYPE type = UPDATE_SCD;
    doc.eraseProperty(B5MHelper::GetSPTPropertyName());
    doc.eraseProperty(B5MHelper::GetBrandPropertyName());
    doc.eraseProperty("Attribute");
    Document::doc_prop_value_strtype category;
    doc.getProperty("Category", category);
    if(category.empty()&&omapper_!=NULL)
    {
        OMap_(*omapper_, doc);
    }
    //Document::doc_prop_value_strtype title;
    //doc.getProperty("Title", title);
    std::string title;
    doc.getString("Title", title);
    //std::string stitle = propstr_to_str(title);
    //brand.convertString(sbrand, UString::UTF_8);
    //std::cerr<<"[ABRAND]"<<sbrand<<std::endl;
    std::string sdocid;
    doc.getString("DOCID", sdocid);
    std::string spid;
    std::string old_spid;
    bool is_human_edit = false;
    if(odb_->get(sdocid, spid)) 
    {
        OfferDb::FlagType flag = 0;
        odb_->get_flag(sdocid, flag);
        if(flag==1)
        {
            is_human_edit = true;
        }
    }
    old_spid = spid;
    bool need_do_match = force_match? true:false;
    if(is_human_edit)
    {
        need_do_match = false;
    }
    else if(mode_>1)
    {
        need_do_match = true;
    }
    else if(spid.empty())
    {
        need_do_match = true;
    }
    ProductMatcher::Product product;
    if(need_do_match)
    {
        matcher_->Process(doc, product, true);
    }
    else
    {
        if(!spid.empty()&&spid!=sdocid)//maybe a book
        {
            product.spid = spid;
        }
        matcher_->GetProduct(spid, product);
    }
    if(!product.spid.empty())
    {
        //has SPU matched
        spid = product.spid;
        if(!title.empty()) 
        {
            //if(category.empty())
            //{
                //category = str_to_propstr(product.scategory);
                //doc.property("Category") = category;
                //if(!product.fcategory.empty())
                //{
                    //Document::doc_prop_value_strtype front(str_to_propstr(product.fcategory));
                    //doc.property(tcp) = front;
                //}
            //}
            if(!product.display_attributes.empty()||!product.filter_attributes.empty())
            {
                if(!product.display_attributes.empty())
                {
                    doc.property("DisplayAttribute") = ustr_to_propstr(product.display_attributes);
                }
                if(!product.filter_attributes.empty())
                {
                    doc.property("Attribute") = ustr_to_propstr(product.filter_attributes);
                    doc.property("FilterAttribute") = ustr_to_propstr(product.filter_attributes);
                }
            }
            else
            {
                const std::vector<ProductMatcher::Attribute>& attributes = product.attributes;
                const std::vector<ProductMatcher::Attribute>& dattributes = product.dattributes;
                if(!attributes.empty()||!dattributes.empty())
                {
                    if(!dattributes.empty())
                    {
                        doc.property("Attribute") = ustr_to_propstr(ProductMatcher::AttributesText(dattributes));
                        doc.property("FilterAttribute") = ustr_to_propstr(ProductMatcher::AttributesText(dattributes));
                    }
                    else if(!attributes.empty())
                    {
                        doc.property("Attribute") = ustr_to_propstr(ProductMatcher::AttributesText(attributes));
                        doc.property("FilterAttribute") = ustr_to_propstr(ProductMatcher::AttributesText(attributes));
                    }
                    //else
                    //{
                        //std::vector<ProductMatcher::Attribute> eattributes;
                        //UString attrib_ustr;
                        //doc.getProperty("Attribute", attrib_ustr);
                        //std::string attrib_str;
                        //attrib_ustr.convertString(attrib_str, UString::UTF_8);
                        //boost::algorithm::trim(attrib_str);
                        //if(!attrib_str.empty())
                        //{
                            //ProductMatcher::ParseAttributes(attrib_ustr, eattributes);
                        //}
                        //std::vector<ProductMatcher::Attribute> new_attributes(attributes);
                        ////ProductMatcher::MergeAttributes(new_attributes, dattributes);
                        //ProductMatcher::MergeAttributes(new_attributes, eattributes);
                        //doc.property("Attribute") = ProductMatcher::AttributesText(new_attributes); 
                    //}
                }
            }
        }
        match_ofs_<<sdocid<<","<<spid<<","<<title<<"\t["<<product.stitle<<"]"<<std::endl;
    }
    else
    {
        spid = sdocid;
    }
    if(!product.stitle.empty() && !title.empty())
    {
        doc.property(B5MHelper::GetSPTPropertyName()) = str_to_propstr(product.stitle);
    }
    if(!product.sbrand.empty() && !title.empty())
    {
        doc.property(B5MHelper::GetBrandPropertyName()) = str_to_propstr(product.sbrand);
    }
    if(old_spid!=spid)
    {
        odb_->insert(sdocid, spid);
        cmatch_ofs_<<sdocid<<","<<spid<<","<<old_spid<<std::endl;
    }
    doc.property("uuid") = str_to_propstr(spid);
    if(sorter_!=NULL)
    {
        if(old_spid!=spid&&!old_spid.empty())
        {
            ScdDocument old_doc;
            old_doc.property("DOCID") = str_to_propstr(sdocid, UString::UTF_8);
            old_doc.property("uuid") = str_to_propstr(old_spid, UString::UTF_8);
            old_doc.type=DELETE_SCD;
            //last_ts_?
            sorter_->Append(old_doc, ts_);
        }
        ScdDocument sdoc(doc, type);
        sorter_->Append(sdoc, ts_);
    }
}

bool B5moProcessor::Generate(const std::string& scd_path, const std::string& mdb_instance, const std::string& last_mdb_instance)
{
    if(!odb_->is_open())
    {
        LOG(INFO)<<"open odb..."<<std::endl;
        if(!odb_->open())
        {
            LOG(ERROR)<<"odb open fail"<<std::endl;
            return false;
        }
    }
    namespace bfs = boost::filesystem;
    std::string output_dir = B5MHelper::GetB5moPath(mdb_instance);
    B5MHelper::PrepareEmptyDir(output_dir);
    std::string sorter_path = B5MHelper::GetB5moBlockPath(mdb_instance); 
    B5MHelper::PrepareEmptyDir(sorter_path);
    sorter_ = new B5moSorter(sorter_path, 200000);
    std::string omapper_path = B5MHelper::GetOMapperPath(mdb_instance);
    if(bfs::exists(omapper_path))
    {
        omapper_ = new OriginalMapper();
        omapper_->Open(omapper_path);
    }
    boost::shared_ptr<ScdTypeWriter> writer(new ScdTypeWriter(output_dir));
    ts_ = bfs::path(mdb_instance).filename().string();
    last_ts_="";
    if(!last_mdb_instance.empty())
    {
        last_ts_ = bfs::path(last_mdb_instance).filename().string();
        std::string last_omapper_path = B5MHelper::GetOMapperPath(last_mdb_instance);
        if(bfs::exists(last_omapper_path))
        {
            OriginalMapper last_omapper;
            last_omapper.Open(last_omapper_path);
            if(bfs::exists(omapper_path))
            {
                last_omapper.Diff(omapper_path);
                if(last_omapper.Size()>0)
                {
                    std::string last_mirror = B5MHelper::GetB5moMirrorPath(last_mdb_instance);
                    std::string last_block = last_mirror+"/block";
                    std::ifstream ifs(last_block.c_str());
                    std::string line;
                    Json::Reader json_reader;
                    while( getline(ifs, line))
                    {
                        boost::algorithm::trim(line);
                        B5moSorter::Value value;
                        if(!value.Parse(line, &json_reader)) continue;
                        if(!OMap_(last_omapper, value.doc)) continue;
                        value.doc.type = UPDATE_SCD;
                        ProcessIU_(value.doc, true);
                        writer->Append(value.doc, value.doc.type);
                    }
                    ifs.close();
                }
            }

        }
    }

    //if(!matcher_->IsOpen())
    //{
        //LOG(INFO)<<"open matcher..."<<std::endl;
        //if(!matcher_->Open())
        //{
            //LOG(ERROR)<<"matcher open fail"<<std::endl;
            //return false;
        //}
    //}

    matcher_->SetMatcherOnly(true);
    if(img_server_cfg_)
    {
        LOG(INFO) << "Got Image Server Cfg, begin Init server connection." << std::endl;
        if(!ImageServerClient::Init(*img_server_cfg_))
        {
            LOG(ERROR) << "Image Server Init failed." << std::endl;
            return false;
        }
        if(!ImageServerClient::Test())
        {
            LOG(ERROR) << "Image Server test failed." << std::endl;
            return false;
        }
    }

    std::string match_file = mdb_instance+"/match";
    std::string cmatch_file = mdb_instance+"/cmatch";
    match_ofs_.open(match_file.c_str());
    cmatch_ofs_.open(cmatch_file.c_str());
    //if(!human_match_file_.empty())
    //{
        //odb_->clear_all_flag();
        //std::ifstream ifs(human_match_file_.c_str());
        //std::string line;
        //uint32_t i=0;
        //while( getline(ifs, line))
        //{
            //boost::algorithm::trim(line);
            //++i;
            //if(i%100000==0)
            //{
                //LOG(INFO)<<"human match processing "<<i<<" item"<<std::endl;
            //}
            //boost::algorithm::trim(line);
            //std::vector<std::string> vec;
            //boost::algorithm::split(vec, line, boost::is_any_of(","));
            //if(vec.size()<2) continue;
            //const std::string& soid = vec[0];
            //const std::string& spid = vec[1];
            //std::string old_spid;
            //odb_->get(soid, old_spid);
            //if(spid!=old_spid)
            //{
                //changed_match_.insert(B5MHelper::StringToUint128(soid));
                //cmatch_ofs_<<soid<<","<<spid<<","<<old_spid<<std::endl;
                //odb_->insert(soid, spid);
            //}
            //odb_->insert_flag(soid, 1);
        //}
        //ifs.close();
        //odb_->flush();
    //}

    ScdDocProcessor::ProcessorType p = boost::bind(&B5moProcessor::Process, this, _1, _2);
    ScdDocProcessor sd_processor(p);
    sd_processor.AddInput(scd_path);
    sd_processor.SetOutput(writer);
    sd_processor.Process();
    match_ofs_.close();
    cmatch_ofs_.close();
    odb_->flush();
    //bdb_->flush();
    if(sorter_!=NULL)
    {
        sorter_->StageOne();
        delete sorter_;
        sorter_ = NULL;
    }
    if(omapper_!=NULL)
    {
        delete omapper_;
        omapper_ = NULL;
    }
    //if(!changed_match_.empty())
    //{
        //if(last_mdb_instance.empty())
        //{
            //LOG(ERROR)<<"pid changed oid not empty but last mdb instance not specify"<<std::endl;
        //}
        //else
        //{
            //std::vector<std::string> last_b5mo_scd_list;
            //std::string last_b5mo_mirror_scd_path = B5MHelper::GetB5moMirrorPath(last_mdb_instance);
            //B5MHelper::GetScdList(last_b5mo_mirror_scd_path, last_b5mo_scd_list);
            //if(last_b5mo_scd_list.size()!=1)
            //{
                //LOG(ERROR)<<"last b5mo scd size should be 1"<<std::endl;
            //}
            //else
            //{
                //ScdWriter edit_u(B5MHelper::GetB5moPath(mdb_instance), UPDATE_SCD);
                //std::string scd_file = last_b5mo_scd_list[0];
                //ScdParser parser(izenelib::util::UString::UTF_8);
                //parser.load(scd_file);
                //uint32_t n=0;
                //for( ScdParser::iterator doc_iter = parser.begin();
                  //doc_iter!= parser.end(); ++doc_iter, ++n)
                //{
                    //if(changed_match_.empty()) break;
                    //if(n%10000==0)
                    //{
                        //LOG(INFO)<<"Last b5mo mirror find Documents "<<n<<std::endl;
                    //}
                    //std::string soldpid;
                    //Document doc;
                    //SCDDoc& scddoc = *(*doc_iter);
                    //SCDDoc::iterator p = scddoc.begin();
                    //for(; p!=scddoc.end(); ++p)
                    //{
                        //const std::string& property_name = p->first;
                        //doc.property(property_name) = p->second;
                    //}
                    //std::string sdocid;
                    //doc.getString("DOCID", sdocid);
                    //uint128_t oid = B5MHelper::StringToUint128(sdocid);
                    //if(changed_match_.find(oid)!=changed_match_.end())
                    //{
                        //std::string spid;
                        //if(odb_->get(sdocid, spid))
                        //{
                            //doc.property("uuid") = spid;
                            //edit_u.Append(doc);
                        //}
                        //changed_match_.erase(oid);
                    //}
                //}
                //edit_u.Close();

            //}
        //}
    //}
    return true;
}

bool B5moProcessor::OMap_(const OriginalMapper& omapper, Document& doc) const
{
    std::string source;
    std::string original_category;
    doc.getString("Source", source);
    doc.getString("OriginalCategory", original_category);
    if(source.empty()||original_category.empty()) return false;
    std::string category;
    if(omapper.Map(source, original_category, category))
    {
        doc.property("Category") = str_to_propstr(category);
        return true;
    }
    return false;
}

