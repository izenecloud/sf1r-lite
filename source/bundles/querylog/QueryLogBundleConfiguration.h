#ifndef QUERYLOG_BUNDLE_CONFIGURATION_H
#define QUERYLOG_BUNDLE_CONFIGURATION_H

#include <util/osgi/BundleConfiguration.h>

#include <string>

namespace sf1r
{
class QueryLogBundleConfiguration : public ::izenelib::osgi::BundleConfiguration
{
public:
    QueryLogBundleConfiguration();

    uint32_t log_days;

    std::string basepath;

    bool query_correction_enableEK;
	
    bool query_correction_enableCN;

    uint32_t autofill_num;

    std::string cronIndexRecommend_;
};

}

#endif

