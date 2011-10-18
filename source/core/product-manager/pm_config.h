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
    std::string price_property_name;
    std::string docid_property_name;
    std::string itemcount_property_name;
    std::string uuid_property_name;
    
    static PMConfig GetDefaultPMConfig()
    {
        PMConfig config;
        config.price_property_name = "Price";
        config.docid_property_name = "DOCID";
        config.itemcount_property_name = "itemcount";
        config.uuid_property_name = "uuid";
        return config;
    }
};



}

#endif

