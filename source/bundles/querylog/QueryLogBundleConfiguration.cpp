#include "QueryLogBundleConfiguration.h"

namespace sf1r
{
QueryLogBundleConfiguration::QueryLogBundleConfiguration()
    : ::izenelib::osgi::BundleConfiguration("QueryLogBundle", "QueryLogBundleActivator" )
    , enable_worker_(false)
{
}
}

