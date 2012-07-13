///
/// @file GroupManager.h
/// @brief manage forward index for string property values (doc id => value id)
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-03-22
///

#ifndef SF1R_GROUP_MANAGER_H_
#define SF1R_GROUP_MANAGER_H_

#include "PropValueTable.h"
#include "DateGroupTable.h"
#include "../faceted-submanager/ontology_rep.h"
#include <configuration-manager/GroupConfig.h>

#include <vector>
#include <string>
#include <map>

namespace sf1r
{
class DocumentManager;
}

NS_FACETED_BEGIN
class DateStrParser;

class GroupManager {
public:
    typedef std::map<std::string, PropValueTable> StrPropMap;
    typedef std::map<std::string, DateGroupTable> DatePropMap;

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

    const PropValueTable* getPropValueTable(const std::string& propName) const
    {
        StrPropMap::const_iterator it = strPropMap_.find(propName);
        if (it != strPropMap_.end())
        {
            return &(it->second);
        }
        return NULL;
    }

    PropValueTable* getPropValueTable(const std::string& propName)
    {
        StrPropMap::iterator it = strPropMap_.find(propName);
        if (it != strPropMap_.end())
        {
            return &(it->second);
        }
        return NULL;
    }

    const DateGroupTable* getDateGroupTable(const std::string& propName) const
    {
        DatePropMap::const_iterator it = datePropMap_.find(propName);
        if (it != datePropMap_.end())
        {
            return &(it->second);
        }
        return NULL;
    }

    DateGroupTable* getDateGroupTable(const std::string& propName)
    {
        DatePropMap::iterator it = datePropMap_.find(propName);
        if (it != datePropMap_.end())
        {
            return &(it->second);
        }
        return NULL;
    }

private:
    bool createPropValueTable_(const std::string& propName);
    bool createDateGroupTable_(const std::string& propName);

    void buildStrPropForCollection_(PropValueTable& pvTable);
    void buildStrPropForDoc_(
        docid_t docId,
        const std::string& propName,
        PropValueTable& pvTable
    );

    void buildDatePropForCollection_(DateGroupTable& dateTable);
    void buildDatePropForDoc_(
        docid_t docId,
        const std::string& propName,
        DateGroupTable& dateTable
    );

private:
    sf1r::DocumentManager* documentManager_;
    std::string dirPath_;
    DateStrParser& dateStrParser_;

    StrPropMap strPropMap_;
    DatePropMap datePropMap_;
};

NS_FACETED_END

#endif 
