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
#include <configuration-manager/GroupConfig.h>
#include <util/ustring/UString.h>

#include <vector>
#include <string>
#include <map>

namespace sf1r
{
class DocumentManager;
}

NS_FACETED_BEGIN

class GroupLabel;

class GroupManager {
public:
    GroupManager(
        DocumentManager* documentManager,
        const std::string& dirPath
    );

    /**
     * @brief Open the properties which need group result.
     * @param configVec the property configs
     * @return true for success, false for failure
     */
    bool open(const std::vector<GroupConfig>& configVec);

    /**
     * @brief Build group index data for the whole collection.
     * @return true for success, false for failure
     */
    bool processCollection();

    /**
     * @brief Get group representation for a property list.
     * @param docIdList a list of doc id, in which doc count is calculated for each property value
     * @param groupPropertyList a list of property name.
     * @param groupLabel check each doc whether belongs to the group labels selected
     * @param groupRep a list, each element is a label tree for a property,
     *                 each label contains doc count for a property value.
     * @return true for success, false for failure.
     */
    bool getGroupRep(
        const std::vector<unsigned int>& docIdList,
        const std::vector<std::string>& groupPropertyList,
        const GroupLabel* groupLabel,
        faceted::OntologyRep& groupRep
    );

    /**
     * @brief Create the instance to check whether doc belongs to group labels.
     * @param groupLabelList a label list, each label is a pair of property name and property value
     * @return the instance used to check conditions
     * @note the caller is responsible to delete the instance returned
     */
    GroupLabel* createGroupLabel(
        const std::vector<std::pair<std::string, std::string> >& groupLabelList
    ) const;

private:
    /**
     * @brief Get group representation by reading property value from document manager.
     */
    bool getGroupRepFromDocumentManager(
        const std::vector<unsigned int>& docIdList,
        const std::vector<std::string>& groupPropertyList,
        const std::vector<std::pair<std::string, std::string> >& groupLabelList,
        faceted::OntologyRep& groupRep
    );

private:
    sf1r::DocumentManager* documentManager_;
    std::string dirPath_;

    typedef std::map<std::string, PropValueTable> PropValueMap;
    PropValueMap propValueMap_;

    friend class PropertyDiversityReranker;
};

NS_FACETED_END

#endif 
