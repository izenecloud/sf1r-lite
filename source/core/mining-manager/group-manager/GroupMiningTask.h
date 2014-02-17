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
#include <common/TableSwapper.h>
#include <string>
#include <vector>
#include <map>
#include <glog/logging.h>


NS_FACETED_BEGIN

class DateStrParser;
using izenelib::util::UString;

template<class TableType>
class GroupMiningTask: public MiningTask
{
public:
    GroupMiningTask(
        DocumentManager& documentManager
        , const GroupConfigMap& groupConfigMap
        , TableType& propValueTable)
        : documentManager_(documentManager)
        , groupConfigMap_(groupConfigMap)
        , propName_(propValueTable.propName())
        , swapper_(propValueTable)
        , dateStrParser_(*DateStrParser::get())
    {}

    ~GroupMiningTask() {}

    bool buildDocument(docid_t docID, const Document& doc)
    {
        Document::doc_prop_value_strtype propValue;
        doc.getProperty(propName_, propValue);
        buildDoc_(docID, propstr_to_ustr(propValue));
        return true;
    }

    bool preProcess(int64_t timestamp)
    {
        docid_t startDocId = swapper_.reader.docIdNum();
        bool isRebuild = isRebuildProp_(propName_);

        if (isRebuild)
        {
            startDocId = 1;
        }

        const docid_t endDocId = documentManager_.getMaxDocId();
        if (startDocId > endDocId)
            return false;

        const docid_t newSize = endDocId + 1;
        if (isRebuild)
        {
            LOG(INFO) << "**clear old group data to rebuild property: "
                      << propName_ << endl;

            swapper_.writer = TableType(swapper_.reader.dirPath(),
                                        propName_);
            swapper_.writer.resize(newSize);
        }
        else
        {
            swapper_.copy(newSize);
        }

        cout << propName_ << ", ";
        return true;
    }

    bool postProcess()
    {
        if (!swapper_.writer.flush())
        {
            LOG(ERROR) << "propTable.flush() failed, property name: " << propName_;
            return false;
        }

        swapper_.swap();
        return true;
    }

    docid_t getLastDocId()
    {
        return swapper_.reader.docIdNum();
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
        const izenelib::util::UString& propValue);

private:
    DocumentManager& documentManager_;
    const GroupConfigMap& groupConfigMap_;
    const std::string propName_;
    TableSwapper<TableType> swapper_;
    DateStrParser& dateStrParser_;
};

template<>
void GroupMiningTask<DateGroupTable>::buildDoc_(
    docid_t docId,
    const izenelib::util::UString& propValue)
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
        swapper_.writer.setDateSet(docId, dateSet);
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
    const izenelib::util::UString& propValue)
{
    std::vector<PropValueTable::pvid_t> propIdList;

    std::vector<vector<izenelib::util::UString> > groupPaths;
    split_group_path(propValue, groupPaths);

    try
    {
        for (std::vector<vector<izenelib::util::UString> >::const_iterator pathIt = groupPaths.begin();
                pathIt != groupPaths.end(); ++pathIt)
        {

            PropValueTable::pvid_t pvId =
                    swapper_.writer.insertPropValueId(*pathIt);
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
        swapper_.writer.setPropIdList(docId, propIdList);
    }
    catch (MiningException& e)
    {
        LOG(ERROR) << "exception: " << e.what()
                   << ", doc id: " << docId;
    }
}

NS_FACETED_END
#endif
