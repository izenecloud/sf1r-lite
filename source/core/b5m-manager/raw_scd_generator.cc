#include "raw_scd_generator.h"
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
#include <am/sequence_file/ssfr.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <glog/logging.h>

using namespace sf1r;

RawScdGenerator::RawScdGenerator(OfferDb* odb, int mode)
:odb_(odb), mode_(mode)
{
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
    std::string spid;
    bool got_pid = false;
    if(odb_->get(sdocid, spid)) got_pid = true;
    if(type!=DELETE_SCD)
    {
        if(got_pid)
        {
            doc.property("uuid") = UString(spid, UString::UTF_8);
        }
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
        if(!odb_->open())
        {
            LOG(ERROR)<<"odb open fail"<<std::endl;
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

