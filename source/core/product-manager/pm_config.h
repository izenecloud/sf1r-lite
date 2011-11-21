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
    bool enablePH;
    std::string price_property_name;
    std::string date_property_name;
    std::string docid_property_name;
    std::string itemcount_property_name;
    std::string uuid_property_name;
    std::string backup_path;

    PMConfig()
        : enablePH(false)
        , price_property_name("Price")
        , date_property_name("Date")
        , docid_property_name("DOCID")
        , itemcount_property_name("itemcount")
        , uuid_property_name("uuid")
        , backup_path()
    {
    }
};

}

#endif
