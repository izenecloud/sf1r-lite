#ifndef SF1V5_QUERY_MANAGER_QUERY_PAGE_IDENTITY_H
#define SF1V5_QUERY_MANAGER_QUERY_PAGE_IDENTITY_H
/**
 * @file query-manager/QueryPageIdentity.h
 * @author Ian Yang
 * @date Created <2009-09-29 17:18:32>
 * @date Updated <2009-09-30 09:51:56>
 * @brief identity for a page in a query
 */
#include "QueryIdentity.h"

namespace sf1r {

struct QueryPageIdentity
{
    QueryIdentity queryIdentity;
    PageInfo pageInfo;

    DATA_IO_LOAD_SAVE(QueryPageIdentity, &queryIdentity&pageInfo);

    template<class Archive>
    void serialize(Archive& ar, const unsigned version)
    {
        ar & queryIdentity;
        ar & pageInfo;
    }
};

void makeQueryPageIdentity(
    QueryPageIdentity& identity,
    const KeywordSearchActionItem& item
);

inline bool operator==(const QueryPageIdentity& a,
                       const QueryPageIdentity& b)
{
    return a.pageInfo == b.pageInfo
        && a.queryIdentity == b.queryIdentity;
}
inline bool operator!=(const QueryPageIdentity& a,
                       const QueryPageIdentity& b)
{
    return !(a == b);
}

} // namespace sf1r
#endif // SF1V5_QUERY_MANAGER_QUERY_PAGE_IDENTITY_H
