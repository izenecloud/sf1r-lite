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
#include "../MiningTask.h"
#include "GroupMiningTask.h"
#include "../faceted-submanager/ontology_rep.h"
#include <configuration-manager/GroupConfig.h>
#include <document-manager/DocumentManager.h>

#include <vector>
#include <string>
#include <map>
#include <glog/logging.h>

NS_FACETED_BEGIN
class DateStrParser;

class GroupManager
{
public:
    typedef std::map<std::string, PropValueTable> StrPropMap;
    typedef std::map<std::string, DateGroupTable> DatePropMap;

    GroupManager(
        const GroupConfigMap& groupConfigMap,
        DocumentManager& documentManager,
        const std::string& dirPath);
    ~GroupManager();

    /**
     * @brief Open the properties which need group result.
     * @return true for success, false for failure
     */
    bool open();

    /**
     * @brief Build group index data for the whole collection.
     * @return true for success, false for failure
     */
    bool processCollection();

    const std::vector<MiningTask*> getGroupMiningTask()
    {
        return groupMiningTaskList_;
    }

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

    template <class PropTableType>
    void buildCollection_(PropTableType& propTable);

    void buildDoc_(
            docid_t docId,
            const std::string& propName,
            PropValueTable& pvTable);

    void buildDoc_(
            docid_t docId,
            const std::string& propName,
            DateGroupTable& dateTable);

    bool isRebuildProp_(const std::string& propName) const;

private:
    const GroupConfigMap& groupConfigMap_;
    DocumentManager& documentManager_;
    const std::string dirPath_;
    DateStrParser& dateStrParser_;

    StrPropMap strPropMap_;
    DatePropMap datePropMap_;
    std::vector<MiningTask*> groupMiningTaskList_;
};

template <class PropTableType>
void GroupManager::buildCollection_(PropTableType& propTable)
{
    const std::string& propName = propTable.propName();

    if (isRebuildProp_(propName))
    {
        LOG(INFO) << "clear old group data to rebuild property: " << propName;
        propTable.clear();
    }

    const docid_t startDocId = propTable.docIdNum();
    const docid_t endDocId = documentManager_.getMaxDocId();
    assert(startDocId && "docid 0 should have been reserved in PropValueTable constructor");

    if (startDocId > endDocId)
        return;

    LOG(INFO) << "start building property: " << propName
              << ", start doc id: " << startDocId
              << ", end doc id: " << endDocId;

    propTable.reserveDocIdNum(endDocId + 1);

    for (docid_t docId = startDocId; docId <= endDocId; ++docId)
    {
        if (docId % 100000 == 0)
        {
            std::cout << "\rinserting doc id: " << docId << "\t" << std::flush;
        }

        buildDoc_(docId, propName, propTable);
    }
    std::cout << "\rinserting doc id: " << endDocId << "\t" << std::endl;

    if (!propTable.flush())
    {
        LOG(ERROR) << "propTable.flush() failed, property name: " << propName;
    }
}

NS_FACETED_END

#endif
