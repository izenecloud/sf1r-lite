#ifndef QUERYLOG_BUNDLE_CONFIGURATION_H
#define QUERYLOG_BUNDLE_CONFIGURATION_H

#include <util/osgi/BundleConfiguration.h>
#include <common/inttypes.h> // uint32_t

#include <string>

namespace sf1r
{
class QueryLogBundleConfiguration : public ::izenelib::osgi::BundleConfiguration
{
public:
    QueryLogBundleConfiguration();

    uint32_t update_time;

    uint32_t log_days;

    std::string basepath;

    bool query_correction_enableEK;
	
    bool query_correction_enableCN;

    uint32_t autofill_num;

    std::string cronIndexRecommend_;

    std::string resource_dir_;

    bool enable_worker_;
};

}

#endif

