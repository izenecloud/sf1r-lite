#ifndef SF1R_PRODUCTMANAGER_PMCONFIG_H
#define SF1R_PRODUCTMANAGER_PMCONFIG_H

#include <sstream>
#include <common/type_defs.h>

#include "pm_def.h"
#include "pm_types.h"

namespace sf1r
{

struct PMConfig
{
    bool enable_price_trend;
    bool enable_clustering_algo;
    uint32_t algo_fixk;
    std::string price_property_name;
    std::string date_property_name;
    std::string docid_property_name;
    std::string itemcount_property_name;
    std::string uuid_property_name;
    std::string olduuid_property_name;
    std::string oldofferids_property_name;
    std::string title_property_name;
    std::string category_property_name;
    std::string source_property_name;
    std::string city_property_name;
    std::string manmade_property_name;
    std::string backup_path;
    std::string uuid_map_path;
    std::vector<std::string> group_property_names;
    std::vector<uint32_t> time_interval_days;

    PMConfig()
        : enable_price_trend(false)
        , enable_clustering_algo(false)
        , algo_fixk(0)
        , price_property_name("Price")
        , date_property_name("DATE")
        , docid_property_name("DOCID")
        , itemcount_property_name("itemcount")
        , uuid_property_name("uuid")
        , olduuid_property_name("olduuid")
        , oldofferids_property_name("OldOfferIds")
        , title_property_name("Title")
        , category_property_name("Category")
        , source_property_name("Source")
        , city_property_name("City")
        , manmade_property_name("manmade")
    {
    }
};

}

#endif
