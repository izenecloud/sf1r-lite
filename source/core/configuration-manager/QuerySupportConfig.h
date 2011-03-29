
#ifndef SF1V5_QUERY_SUPPORT_CONFIG_H_
#define SF1V5_QUERY_SUPPORT_CONFIG_H_

#include <configuration-manager/LAConfigUnit.h>

#include <boost/serialization/map.hpp>

#include <string>
#include <map>

namespace sf1r
{

class QuerySupportConfig
{
public:
    //------------------------  PRIVATE MEMBER VARIABLES  ------------------------

    uint32_t update_time;

    uint32_t log_days;

    std::string basepath;

    std::string logpath;

    std::string scdpath;

    bool query_correction_enableEK;

    bool query_correction_enableCN;

    uint32_t autofill_num;

    std::string kpeproperty;

    std::string kpebindir;
};



} // namespace

#endif  //_QUERY_MANAGER_CONFIG_H_

// eof
