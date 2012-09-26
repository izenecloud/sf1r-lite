#include "b5mo_processor.h"
#include "log_server_client.h"
#include "image_server_client.h"
#include "b5m_types.h"
#include "b5m_helper.h"
#include "b5m_mode.h"
#include <common/ScdParser.h>
#include <common/ScdWriter.h>
#include <common/ScdMerger.h>
#include <common/ScdWriterController.h>
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
:odb_(odb), matcher_(matcher), mode_(mode), img_server_cfg_(img_server_config)
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

void B5moProcessor::Process(Document& doc, int& type)
{
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
    UString uprice;
    if(doc.getProperty("Price", uprice))
    {
        ProductPrice pp;
        pp.Parse(uprice);
        doc.property("Price") = pp.ToUString();
    }

    //set mobile tag
    UString usource;
    if(doc.getProperty("Source", usource))
    {
        std::string ssource;
        usource.convertString(ssource, izenelib::util::UString::UTF_8);
        if(mobile_source_.find(ssource)!=mobile_source_.end())
        {
            doc.property("mobile") = (int64_t)1;
        }
    }

    //set color tag
    if(img_server_cfg_)
    {
        std::string img_file;
        doc.getString("Picture", img_file);
        if(!img_file.empty())
        {
            std::string color_str;
            ImageServerClient::GetImageColor(img_file, color_str);
            doc.property("Color") = UString(color_str, UString::UTF_8);
        }
    }

    //set pid(uuid) tag

    std::string sdocid;
    doc.getString("DOCID", sdocid);
    uint128_t oid = B5MHelper::StringToUint128(sdocid);
    if(type!=DELETE_SCD)
    {
        std::string spid;
        bool got_pid = false;
        bool is_human_edit = false;
        if(odb_->get(sdocid, spid)) 
        {
            got_pid = true;
            OfferDb::FlagType flag = 0;
            odb_->get_flag(sdocid, flag);
            if(flag==1)
            {
                is_human_edit = true;
            }
        }
        std::string old_spid = spid;
        if(!got_pid || (mode_>1&&!is_human_edit)) //not got pid or re-match(not human edit)
        {
            ProductMatcher::Product product;
            if(matcher_->GetMatched(doc, product))
            {
                spid = product.spid;
                UString title;
                doc.getProperty("Title", title);
                std::string stitle;
                title.convertString(stitle, UString::UTF_8);
                match_ofs_<<sdocid<<","<<spid<<","<<stitle<<","<<product.stitle<<std::endl;
            }
            else
            {
                spid = sdocid; //get matched pid fail
            }
            odb_->insert(sdocid, spid);
        }
        if(old_spid!=spid)
        {
            cmatch_ofs_<<sdocid<<","<<spid<<","<<old_spid<<std::endl;
        }
        doc.property("uuid") = UString(spid, UString::UTF_8);
    }
    else
    {
        doc.clearExceptDOCID();
    }
    if(!changed_match_.empty())
    {
        changed_match_.erase(oid);
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
    if(!matcher_->IsOpen())
    {
        LOG(INFO)<<"open matcher..."<<std::endl;
        if(!matcher_->Open())
        {
            LOG(ERROR)<<"matcher open fail"<<std::endl;
            return false;
        }
    }

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
    if(!human_match_file_.empty())
    {
        odb_->clear_all_flag();
        std::ifstream ifs(human_match_file_.c_str());
        std::string line;
        uint32_t i=0;
        while( getline(ifs, line))
        {
            boost::algorithm::trim(line);
            ++i;
            if(i%100000==0)
            {
                LOG(INFO)<<"human match processing "<<i<<" item"<<std::endl;
            }
            boost::algorithm::trim(line);
            std::vector<std::string> vec;
            boost::algorithm::split(vec, line, boost::is_any_of(","));
            if(vec.size()<2) continue;
            const std::string& soid = vec[0];
            const std::string& spid = vec[1];
            std::string old_spid;
            odb_->get(soid, old_spid);
            if(spid!=old_spid)
            {
                changed_match_.insert(B5MHelper::StringToUint128(soid));
                cmatch_ofs_<<soid<<","<<spid<<","<<old_spid<<std::endl;
                odb_->insert(soid, spid);
            }
            odb_->insert_flag(soid, 1);
        }
        ifs.close();
        odb_->flush();
    }

    ScdMerger::PropertyConfig config;
    config.output_dir = B5MHelper::GetB5moPath(mdb_instance);
    B5MHelper::PrepareEmptyDir(config.output_dir);
    config.property_name = "DOCID";
    config.merge_function = &ScdMerger::DefaultMergeFunction;
    config.output_function = boost::bind(&B5moProcessor::Process, this, _1, _2);
    config.output_if_no_position = true;
    ScdMerger merger;
    merger.AddPropertyConfig(config);
    merger.AddInput(scd_path);
    merger.Output();
    match_ofs_.close();
    cmatch_ofs_.close();
    odb_->flush();
    if(!changed_match_.empty())
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
                ScdWriter edit_u(B5MHelper::GetB5moPath(mdb_instance), UPDATE_SCD);
                std::string scd_file = last_b5mo_scd_list[0];
                ScdParser parser(izenelib::util::UString::UTF_8);
                parser.load(scd_file);
                uint32_t n=0;
                for( ScdParser::iterator doc_iter = parser.begin();
                  doc_iter!= parser.end(); ++doc_iter, ++n)
                {
                    if(changed_match_.empty()) break;
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
                    std::string sdocid;
                    doc.getString("DOCID", sdocid);
                    uint128_t oid = B5MHelper::StringToUint128(sdocid);
                    if(changed_match_.find(oid)!=changed_match_.end())
                    {
                        std::string spid;
                        if(odb_->get(sdocid, spid))
                        {
                            doc.property("uuid") = spid;
                            edit_u.Append(doc);
                        }
                        changed_match_.erase(oid);
                    }
                }
                edit_u.Close();

            }
        }
    }
    return true;
}

