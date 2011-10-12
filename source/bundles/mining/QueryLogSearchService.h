#ifndef QUERYLOG_BUNDLE_SEARCH_SERVICE_H
#define QUERYLOG_BUNDLE_SEARCH_SERVICE_H

#include <util/osgi/IService.h>
#include <util/ustring/UString.h>
#include <common/sf1_serialization_types.h>

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

namespace sf1r
{
class MiningManager;

class QueryLogSearchService : public ::izenelib::osgi::IService
{
public:
    QueryLogSearchService();

    ~QueryLogSearchService();

public:
    bool getRefinedQuery(const std::string& collectionName, const UString& queryUString, UString& refinedQueryUString);

//     bool getAutoFillList(const izenelib::util::UString& query, std::vector<std::pair<izenelib::util::UString,uint32_t> >& list);

private:
    friend class MiningBundleActivator;
};

}

#endif

