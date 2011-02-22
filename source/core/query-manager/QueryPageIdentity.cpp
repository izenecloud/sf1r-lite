/**
 * @file core/query-manager/QueryPageIdentity.cpp
 * @author Ian Yang
 * @date Created <2009-09-29 17:29:15>
 * @date Updated <2009-09-30 09:53:59>
 */
#include "QueryPageIdentity.h"

namespace sf1r {
void makeQueryPageIdentity(
    QueryPageIdentity& identity,
    const KeywordSearchActionItem& item
)
{
    makeQueryIdentity(identity.queryIdentity, item);
    identity.pageInfo = item.pageInfo_;
}
} // namespace sf1r
