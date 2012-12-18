///
/// @file GroupMiningTaks.h
/// @brief prcessCollection for Group
/// @author hongliang.zhao@b5m.com
/// @date Created 2012.11.28
///
#ifndef GROUP_MINING_TASK_H
#define GROUP_MINING_TASK_H

#include "../MiningTask.h"
#include "PropValueTable.h"
#include "DateGroupTable.h"
#include "DateStrParser.h"
#include "../faceted-submanager/ontology_rep.h"
#include <document-manager/DocumentManager.h>
#include <configuration-manager/GroupConfig.h>
#include <mining-manager/util/split_ustr.h>
#include <string>
#include <vector>
#include <map>
#include <glog/logging.h>


NS_FACETED_BEGIN

class DateStrParser;
using izenelib::util::UString;

template<class PropVauleType>
class GroupMiningTask: public MiningTask
{
public:
    GroupMiningTask(
        DocumentManager& documentManager
        , const GroupConfigMap& groupConfigMap
        , PropVauleType& propValueTable)
        :documentManager_(documentManager)
        , groupConfigMap_(groupConfigMap)
        , propValueTable_(propValueTable)
        , dateStrParser_(*DateStrParser::get())
    {}

    ~GroupMiningTask() {}

    bool buildDocment(docid_t docID, const Document& doc)
    {
        izenelib::util::UString propValue;
        doc.getProperty(propValueTable_.propName(), propValue);
        buildDoc_(docID, propValue, propValueTable_);//return
        return true;
    }

    bool preProcess()
    {
        if (isRebuildProp_(propValueTable_.propName()))
        {
            LOG(INFO) << "**clear old group data to rebuild property: " << propValueTable_.propName() <<endl;
            propValueTable_.clear();
        }
        const docid_t endDocId = documentManager_.getMaxDocId();
        docid_t startDocId = propValueTable_.docIdNum();
        if (startDocId > endDocId)
            return false;
        cout<<propValueTable_.propName()<<", ";
        propValueTable_.reserveDocIdNum(endDocId + 1);
        return true;
    }

    bool postProcess()
    {
        if (!propValueTable_.flush())
        {
            LOG(ERROR) << "propTable.flush() failed, property name: " << propValueTable_.propName();
            return false;
        }
        return true;
    }

    docid_t getLastDocId()
    {
        return propValueTable_.docIdNum();
    }

    bool isRebuildProp_(const std::string& propName) const
    {
        GroupConfigMap::const_iterator it = groupConfigMap_.find(propName);

        if (it != groupConfigMap_.end())
            return it->second.isRebuild();
        return false;
    }

    void buildDoc_(
        docid_t docId,
        const izenelib::util::UString& propValue,
        PropVauleType& propValueTable);

private:
    DocumentManager& documentManager_;
    const GroupConfigMap& groupConfigMap_;
    PropVauleType& propValueTable_;
    DateStrParser& dateStrParser_;
};

template<>
void GroupMiningTask<DateGroupTable>::buildDoc_(
    docid_t docId,
    const izenelib::util::UString& propValue,
    DateGroupTable& propValueTable)
{
    DateGroupTable::DateSet dateSet;

    {
        std::vector<vector<izenelib::util::UString> > groupPaths;
        split_group_path(propValue, groupPaths);

        DateGroupTable::date_t dateValue;
        std::string dateStr;
        std::string errorMsg;

        for (std::vector<vector<izenelib::util::UString> >::const_iterator pathIt = groupPaths.begin();
                pathIt != groupPaths.end(); ++pathIt)
        {
            if (pathIt->empty())
                continue;

            pathIt->front().convertString(dateStr, UString::UTF_8);
            if (dateStrParser_.scdStrToDate(dateStr, dateValue, errorMsg))
            {
                dateSet.insert(dateValue);
            }
            else
            {
                LOG(WARNING) << errorMsg;
            }
        }
    }

    try
    {
        propValueTable.appendDateSet(dateSet);
    }
    catch(MiningException& e)
    {
        LOG(ERROR) << "exception: " << e.what()
                   << ", doc id: " << docId;
    }
}

template<>
void GroupMiningTask<PropValueTable>::buildDoc_(
    docid_t docId,
    const izenelib::util::UString& propValue,
    PropValueTable& propValueTable)
{
    std::vector<PropValueTable::pvid_t> propIdList;

    std::vector<vector<izenelib::util::UString> > groupPaths;
    split_group_path(propValue, groupPaths);

    try
    {
        for (std::vector<vector<izenelib::util::UString> >::const_iterator pathIt = groupPaths.begin();
                pathIt != groupPaths.end(); ++pathIt)
        {

            PropValueTable::pvid_t pvId = propValueTable.insertPropValueId(*pathIt);
            propIdList.push_back(pvId);
        }
    }
    catch (MiningException& e)
    {
        LOG(ERROR) << "exception: " << e.what()
                   << ", doc id: " << docId;
    }
    try
    {
        propValueTable.appendPropIdList(propIdList);
    }
    catch (MiningException& e)
    {
        LOG(ERROR) << "exception: " << e.what()
                   << ", doc id: " << docId;
    }
}

NS_FACETED_END
#endif
