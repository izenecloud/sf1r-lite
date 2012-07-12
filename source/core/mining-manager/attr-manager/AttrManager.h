///
/// @file AttrManager.h
/// @brief calculate doc count for each attribute value
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-06-22
///

#ifndef SF1R_ATTR_MANAGER_H_
#define SF1R_ATTR_MANAGER_H_

#include "../faceted-submanager/ontology_rep.h"
#include "AttrTable.h"
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
        const AttrConfig& attrConfig,
        const std::string& dirPath,
        DocumentManager& documentManager
    );

    /**
     * @brief Open the attribute property.
     * @return true for success, false for failure
     */
    bool open();

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
    void buildDoc_(
        docid_t docId,
        const std::string& propName
    );

private:
    const AttrConfig& attrConfig_;
    std::string dirPath_;

    sf1r::DocumentManager& documentManager_;

    AttrTable attrTable_;
};

NS_FACETED_END

#endif // SF1R_ATTR_MANAGER_H_
