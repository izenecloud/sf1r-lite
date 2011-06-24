#include "group_label.h"

#include <iostream>

#include <glog/logging.h>

using namespace sf1r::faceted;

namespace
{

/**
 * Check whether @p v1 is the parent node of @p v2.
 * @param parentTable table containing parent node for each node, 0 for root node
 * @param v1 node 1
 * @param v2 node 2
 * @return true for @p v1 is the parent node of @p v2, otherwise false is returned,
 *         when @p v1 equals to @p v2, true is returned,
 *         when either @p v1 or @p v2 is zero for invalid id, false is returned.
 */
bool isParentValue(
    const std::vector<PropValueTable::pvid_t>& parentTable,
    PropValueTable::pvid_t v1,
    PropValueTable::pvid_t v2
)
{
    if (v1 == 0 || v2 == 0)
        return false;

    if (v2 >= parentTable.size())
        return (v2 == v1);

    while (v2 != 0)
    {
        if (v2 == v1)
            return true;

        v2 = parentTable[v2];
    }

    return false;
}

}

bool GroupLabel::checkDoc(docid_t docId) const
{
    bool result = true;
    for (CondList::const_iterator condIt = condList_.begin();
            condIt != condList_.end(); ++condIt)
    {
        const PropValueTable* condPVTable = condIt->first;
        const std::vector<PropValueTable::pvid_t>& condIdTable = condPVTable->propIdTable();
        if (docId >= condIdTable.size()
                || isParentValue(condPVTable->parentIdTable(), condIt->second, condIdTable[docId]) == false)
        {
            result = false;
            break;
        }
    }

    return result;
}

bool GroupLabel::checkDocExceptProp(
    docid_t docId, 
    const std::string& propName
) const
{
    bool result = true;
    for (CondList::const_iterator condIt = condList_.begin();
            condIt != condList_.end(); ++condIt)
    {
        const PropValueTable* condPVTable = condIt->first;
        if (condPVTable->propName() != propName)
        {
            const std::vector<PropValueTable::pvid_t>& condIdTable = condPVTable->propIdTable();
            if (docId >= condIdTable.size()
                    || isParentValue(condPVTable->parentIdTable(), condIt->second, condIdTable[docId]) == false)
            {
                result = false;
                break;
            }
        }
    }

    return result;
}
