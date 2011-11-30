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
    std::string price_property_name;
    std::string date_property_name;
    std::string docid_property_name;
    std::string itemcount_property_name;
    std::string uuid_property_name;
    std::string backup_path;
    std::vector<std::string> group_property_names;
    std::vector<uint32_t> time_interval_days;

    PMConfig()
        : enable_price_trend(false)
        , price_property_name("Price")
        , date_property_name("DATE")
        , docid_property_name("DOCID")
        , itemcount_property_name("itemcount")
        , uuid_property_name("uuid")
    {
    }
};

}

#endif
