///
/// @file attr_manager.h
/// @brief calculate doc count for each attribute value
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-06-22
///

#ifndef SF1R_ATTR_MANAGER_H_
#define SF1R_ATTR_MANAGER_H_

#include "ontology_rep.h"
#include "attr_table.h"
#include <configuration-manager/AttrConfig.h>

#include <string>

namespace sf1r
{
class DocumentManager;
}

NS_FACETED_BEGIN

class AttrManager {
public:
    AttrManager(
        DocumentManager* documentManager,
        const std::string& dirPath
    );

    /**
     * @brief Open the property which need group result for attribute value.
     * @param attrConfig the property config
     * @return true for success, false for failure
     */
    bool open(const AttrConfig& attrConfig);

    /**
     * @brief Build group index data for the whole collection.
     * @return true for success, false for failure
     */
    bool processCollection();

    const AttrTable& getAttrTable() const
    {
        return attrTable_;
    }

private:
    sf1r::DocumentManager* documentManager_;
    std::string dirPath_;

    AttrTable attrTable_;
};

NS_FACETED_END

#endif // SF1R_ATTR_MANAGER_H_
