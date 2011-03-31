#ifndef QUERYLOG_BUNDLE_SEARCH_SERVICE_H
#define QUERYLOG_BUNDLE_SEARCH_SERVICE_H

#include <util/osgi/IService.h>

#include <common/sf1_serialization_types.h>
#include <common/type_defs.h>

#include <query-manager/ActionItem.h>
#include <mining-manager/MiningManager.h>

#include <boost/shared_ptr.hpp>

namespace sf1r
{
class QueryLogSearchService : public ::izenelib::osgi::IService
{
public:
    QueryLogSearchService();

    ~QueryLogSearchService();

public:
    bool getRefinedQuery(const std::string& collectionName, const UString& queryUString, UString& refinedQueryUString);

    bool getAutoFillList(const izenelib::util::UString& query, std::vector<izenelib::util::UString>& list);

private:
    boost::shared_ptr<MiningManager> miningManager_;

    friend class QueryLogBundleActivator;
};

}

#endif

