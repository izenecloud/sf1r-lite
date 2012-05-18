#include "b5mp_processor.h"
#include "b5m_types.h"
#include "b5m_helper.h"
#include <types.h>
#include <common/ScdParser.h>
#include <common/Utilities.h>
#include <product-manager/product_price.h>
#include <product-manager/uuid_generator.h>
#include <am/sequence_file/ssfr.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>

using namespace sf1r;

B5mpProcessor::B5mpProcessor(const std::string& mdb_instance, const std::string& last_mdb_instance)
:mdb_instance_(mdb_instance), last_mdb_instance_(last_mdb_instance), po_map_writer_(NULL)
{
}

bool B5mpProcessor::Generate()
{
    ScdMerger::PropertyConfig config;
    config.output_dir = B5MHelper::GetB5mpPath(mdb_instance_);
    B5MHelper::PrepareEmptyDir(config.output_dir);
    config.property_name = "uuid";
    config.merge_function = boost::bind( &B5mpProcessor::ProductMerge_, this, _1, _2);
    config.output_function = boost::bind( &B5mpProcessor::ProductOutput_, this, _1, _2);
    config.output_if_no_position = false;
    std::string b5mo_mirror = B5MHelper::GetB5moMirrorPath(mdb_instance_);
    B5MHelper::PrepareEmptyDir(b5mo_mirror);
    ScdMerger merger(b5mo_mirror, true);
    merger.AddPropertyConfig(config);
    if(!last_mdb_instance_.empty())
    {
        std::string mirror = B5MHelper::GetB5moMirrorPath(last_mdb_instance_);
        ScdMerger::InputSource is;
        is.scd_path = mirror;
        is.position = false;
        merger.AddInput(is);
    }
    ScdMerger::InputSource is;
    is.scd_path = B5MHelper::GetB5moPath(mdb_instance_);
    is.position = true;
    merger.AddInput(is);

    po_map_writer_ = new PoMapWriter(B5MHelper::GetPoMapPath(mdb_instance_));
    po_map_writer_->Open();
    merger.Output();
    po_map_writer_->Close();
    delete po_map_writer_;
    po_map_writer_ = NULL;
    return true;
}


void B5mpProcessor::ProductMerge_(ScdMerger::ValueType& value, const ScdMerger::ValueType& another_value)
{
    //value is pdoc or empty, another_value is odoc
    ProductProperty pp;
    if(!value.empty())
    {
        pp.Parse(value.doc);
    }
    ProductProperty another;
    another.Parse(another_value.doc);
    pp += another;
    if(another_value.type!=DELETE_SCD)
    {
        if(value.empty() || another.oid==another.pid )
        {
            value.doc = another_value.doc;
        }
        value.type = UPDATE_SCD;
    }
    else
    {
        pp.itemcount -= 1;
    }
    pp.Set(value.doc);

    //add to po_map
    uint128_t ipid = B5MHelper::UStringToUint128(another.pid);
    std::string oid;
    if(another_value.type!=DELETE_SCD)
    {
        another.oid.convertString(oid, izenelib::util::UString::UTF_8);
    }
    po_map_writer_->Append(ipid, oid);

}

void B5mpProcessor::ProductOutput_(Document& doc, int& type)
{
    int64_t itemcount = 0;
    doc.getProperty("itemcount", itemcount);
    if(itemcount<=0)
    {
        type = DELETE_SCD;
        doc.clearExceptDOCID();
    }
}

