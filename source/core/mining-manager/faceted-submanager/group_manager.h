///
/// @file group_manager.h
/// @brief calculate doc count for each property value
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-03-22
///

#ifndef SF1R_GROUP_MANAGER_H_
#define SF1R_GROUP_MANAGER_H_

#include "ontology_rep.h"
#include "prop_value_table.h"
#include <util/ustring/UString.h>

#include <vector>
#include <string>
#include <map>

namespace sf1r
{
class DocumentManager;
}

NS_FACETED_BEGIN

class GroupManager {
public:
    GroupManager(
        DocumentManager* documentManager,
        const std::string& dirPath
    );

    /**
     * @brief Open the properties which need group result.
     * @param properties the property names
     * @return true for success, false for failure
     */
    bool open(const std::vector<std::string>& properties);

    /**
     * @brief Build group index data for the whole collection.
     * @return true for success, false for failure
     */
    bool processCollection();

    /**
     * @brief Get group representation for a property list.
     * @param docIdList a list of doc id, in which doc count is calculated for each property value
     * @param groupPropertyList a list of property name.
     * @param groupRep a list, each element is a label tree for a property,
     *                 each label contains doc count for a property value.
     * @return true for success, false for failure.
     */
    bool getGroupRep(
        const std::vector<unsigned int>& docIdList,
        const std::vector<std::string>& groupPropertyList,
        faceted::OntologyRep& groupRep
    );

private:
    /**
     * @brief Get group representation by reading property value from document manager.
     */
    bool getGroupRepFromDocumentManager(
        const std::vector<unsigned int>& docIdList,
        const std::vector<std::string>& groupPropertyList,
        faceted::OntologyRep& groupRep
    );

    /**
     * @brief Get group representation by reading property value from a table which is pre-loaded in memory.
     */
    bool getGroupRepFromTable(
        const std::vector<unsigned int>& docIdList,
        const std::vector<std::string>& groupPropertyList,
        faceted::OntologyRep& groupRep
    );

private:
    sf1r::DocumentManager* documentManager_;
    std::string dirPath_;

    typedef std::map<std::string, PropValueTable> PropValueMap;
    PropValueMap propValueMap_;
};

NS_FACETED_END

#endif 
