#include "raw_scd_generator.h"
#include "log_server_client.h"
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
#include <am/sequence_file/ssfr.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <glog/logging.h>

using namespace sf1r;

RawScdGenerator::RawScdGenerator(OfferDb* odb, HistoryDB* hdb, int mode, LogServerConnectionConfig* config)
:odb_(odb), historydb_(hdb), mode_(mode), log_server_cfg_(config)
{
}

void RawScdGenerator::LoadMobileSource(const std::string& file)
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

void RawScdGenerator::Process(Document& doc, int& type)
{
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
    std::string sdocid;
    doc.getString("DOCID", sdocid);
    //format Price property
    UString uprice;
    if(doc.getProperty("Price", uprice))
    {
        ProductPrice pp;
        pp.Parse(uprice);
        doc.property("Price") = pp.ToUString();
    }

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
    std::string spid;
    bool got_pid = false;
    if(odb_->get(sdocid, spid)) got_pid = true;

    std::string soldpids;
    if(historydb_)
    {
        historydb_->offer_get(sdocid, soldpids);
    }
    else
    {
        LogServerClient::GetOldUUIDList(sdocid, soldpids);
    }
    if(type!=DELETE_SCD)
    {
        if(got_pid)
        {
            //std::string soldpid;
            //doc.getString("uuid", soldpid);
            //if(!soldpid.empty() && soldpid != spid)
            //{
            //    historydb_->offer_insert(sdocid, soldpid);
            //    historydb_->offer_get(sdocid, soldpids);
            //}
            doc.property("uuid") = UString(spid, UString::UTF_8);
        }
        //std::string solduuid;
        //doc.getString("olduuid", solduuid);
        //if(!soldpid.empty())
        //{
        //    add to historydb_
        //}
        doc.property("olduuid") = UString(soldpids, UString::UTF_8);
    }
    else
    {
        doc.clearExceptDOCID();
    }
}

bool RawScdGenerator::Generate(const std::string& scd_path, const std::string& mdb_instance)
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
    if(historydb_)
    {
        if(!historydb_->is_open())
        {
            LOG(INFO)<<"open history db..."<<std::endl;
            if(!historydb_->open())
            {
                LOG(ERROR)<<"history db open fail"<<std::endl;
                return false;
            }
        }
    }
    else
    {
        if(log_server_cfg_ == NULL)
        {
            LOG(WARNING) << "Log Server config is null." << std::endl;
            return false;
        }
        // use logserver instead of local history 
        if(!LogServerClient::Init(*log_server_cfg_))
        {
            LOG(WARNING) << "Log Server Init failed." << std::endl;
            return false;
        }
        if(!LogServerClient::Test())
        {
            LOG(WARNING) << "log server test failed" << std::endl;
            return false;
        }
    }

    ScdMerger::PropertyConfig config;
    config.output_dir = B5MHelper::GetRawPath(mdb_instance);
    B5MHelper::PrepareEmptyDir(config.output_dir);
    config.property_name = "DOCID";
    config.merge_function = &ScdMerger::DefaultMergeFunction;
    config.output_function = boost::bind(&RawScdGenerator::Process, this, _1, _2);
    config.output_if_no_position = true;
    ScdMerger merger;
    merger.AddPropertyConfig(config);
    merger.AddInput(scd_path);
    merger.Output();
    return true;
}

