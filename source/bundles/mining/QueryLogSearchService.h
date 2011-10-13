#ifndef QUERYLOG_BUNDLE_SEARCH_SERVICE_H
#define QUERYLOG_BUNDLE_SEARCH_SERVICE_H

#include <util/osgi/IService.h>
#include <util/ustring/UString.h>

#include <string>

namespace sf1r
{
class QueryLogSearchService : public ::izenelib::osgi::IService
{
public:
    QueryLogSearchService();

    ~QueryLogSearchService();

    bool getRefinedQuery(const std::string& collectionName, const UString& queryUString, UString& refinedQueryUString);

    static QueryLogSearchService *instance();

private:
    static QueryLogSearchService *serviceInstance_;
};

}

#endif

