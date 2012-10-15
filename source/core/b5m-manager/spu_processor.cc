#include "spu_processor.h"
#include "b5m_types.h"
#include "b5m_helper.h"
#include "b5m_mode.h"
#include <common/ScdParser.h>
#include <common/ScdWriter.h>
#include <common/Utilities.h>
#include <common/ScdMerger.h>
#include <product-manager/product_price.h>
#include <product-manager/uuid_generator.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <glog/logging.h>

using namespace sf1r;

SpuProcessor::SpuProcessor(const std::string& dir)
:dir_(dir)
{
}

void SpuProcessor::Merge_(ScdMerger::ValueType& value, const ScdMerger::ValueType& another_value)
{
    int64_t ptype1 = 0;
    int64_t ptype2 = 0;
    value.doc.getProperty("ptype", ptype1);
    another_value.doc.getProperty("ptype", ptype2);
    if(ptype2>=ptype1)
    {
        value = another_value;
    }
}

void SpuProcessor::Output_(Document& doc, int& type)
{
}

bool SpuProcessor::Upgrade()
{
    std::string new_path = dir_+"/new";
    std::string db_path = dir_+"/db";
    std::string backup_path = dir_+"/backup";
    std::vector<std::string> pathes;
    pathes.push_back(new_path);
    pathes.push_back(db_path);
    pathes.push_back(backup_path);
    for(uint32_t i=0;i<pathes.size();i++)
    {
        if(!boost::filesystem::exists(pathes[i]))
        {
            boost::filesystem::create_directories(pathes[i]);
        }
    }
    std::vector<std::string> new_scd_list;
    std::vector<std::string> db_scd_list;
    B5MHelper::GetIUScdList(new_path, new_scd_list);
    B5MHelper::GetIUScdList(db_path, db_scd_list);
    if(new_scd_list.empty())
    {
        std::cerr<<"new scd empty"<<std::endl;
        return true;
    }
    std::vector<std::string> current_scd_list;
    B5MHelper::GetIUScdList(dir_, current_scd_list);
    ScdMerger::PropertyConfig config;
    config.output_dir = dir_;
    config.property_name = "DOCID";
    config.merge_function = boost::bind(&SpuProcessor::Merge_, this, _1, _2);
    config.output_function = ScdMerger::GetDefaultOutputFunction(false);
    config.output_if_no_position = true;
    ScdMerger merger;
    merger.AddPropertyConfig(config);
    merger.AddInput(db_path);
    merger.AddInput(new_path);
    merger.Output();
    //do backup and clean
    for(uint32_t i=0;i<new_scd_list.size();i++)
    {
        boost::filesystem::remove_all(new_scd_list[i]);
    }
    for(uint32_t i=0;i<current_scd_list.size();i++)
    {
        boost::filesystem::path p(current_scd_list[i]);
        boost::filesystem::path d(backup_path);
        d/=p.filename();
        boost::filesystem::rename(p,d);
    }
    return true;

}

