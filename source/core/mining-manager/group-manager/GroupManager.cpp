#include "GroupManager.h"
#include "DateStrParser.h"
#include <mining-manager/util/split_ustr.h>
#include <mining-manager/util/FSUtil.hpp>
#include <mining-manager/MiningException.hpp>

#include <iostream>
#include <cassert>

using namespace sf1r::faceted;

namespace
{

const izenelib::util::UString::EncodingType ENCODING_TYPE = izenelib::util::UString::UTF_8;

}

GroupManager::GroupManager(
    const GroupConfigMap& groupConfigMap,
    DocumentManager& documentManager,
    const std::string& dirPath)
    : groupConfigMap_(groupConfigMap)
    , documentManager_(documentManager)
    , dirPath_(dirPath)
    , dateStrParser_(*DateStrParser::get())
{
}

bool GroupManager::open()
{
    strPropMap_.clear();
    datePropMap_.clear();

    try
    {
        FSUtil::createDir(dirPath_);
    }
    catch(FileOperationException& e)
    {
        LOG(ERROR) << "exception in FSUtil::createDir: " << e.what();
        return false;
    }

    LOG(INFO) << "Start loading group directory: " << dirPath_;

    for (GroupConfigMap::const_iterator it = groupConfigMap_.begin();
        it != groupConfigMap_.end(); ++it)
    {
        bool result = true;
        const std::string& propName = it->first;
        const GroupConfig& groupConfig = it->second;

        if (groupConfig.isStringType())
        {
            result = createPropValueTable_(propName);
        }
        else if (groupConfig.isDateTimeType())
        {
            result = createDateGroupTable_(propName);
        }

        if (!result)
            return false;
    }

    LOG(INFO) << "End group loading";

    return true;
}

bool GroupManager::createPropValueTable_(const std::string& propName)
{
    std::pair<StrPropMap::iterator, bool> res =
        strPropMap_.insert(StrPropMap::value_type(propName, PropValueTable(dirPath_, propName)));

    if (res.second)
    {
        PropValueTable& pvTable = res.first->second;

        if (!pvTable.open())
        {
            LOG(ERROR) << "PropValueTable::open() failed, property name: " << propName;
            return false;
        }
    }
    else
    {
        LOG(WARNING) << "the group property " << propName << " is opened already.";
    }

    return true;
}

bool GroupManager::createDateGroupTable_(const std::string& propName)
{
    std::pair<DatePropMap::iterator, bool> res =
        datePropMap_.insert(DatePropMap::value_type(propName, DateGroupTable(dirPath_, propName)));

    if (res.second)
    {
        DateGroupTable& dateTable = res.first->second;

        if (!dateTable.open())
        {
            LOG(ERROR) << "DateGroupTable::open() failed, property name: " << propName;
            return false;
        }
    }
    else
    {
        LOG(WARNING) << "the group property " << propName << " is opened already.";
    }

    return true;
}

bool GroupManager::processCollection()
{
    LOG(INFO) << "start building group index data...";

    for (StrPropMap::iterator it = strPropMap_.begin();
        it != strPropMap_.end(); ++it)
    {
        buildCollection_(it->second);
    }

    for (DatePropMap::iterator it = datePropMap_.begin();
        it != datePropMap_.end(); ++it)
    {
        buildCollection_(it->second);
    }

    LOG(INFO) << "finished building group index data";
    return true;
}

bool GroupManager::processCollection1()
{
    LOG(INFO) << "start building group index data";

    buildCollection1_(strPropMap_);

    buildCollection1_(datePropMap_);

    LOG(INFO) << "finished building group index data";
    return true;
}


void GroupManager::buildDoc1_(
    docid_t docId,
    const izenelib::util::UString propValue,
    PropValueTable& pvTable)
{ 
    std::vector<PropValueTable::pvid_t> propIdList;

    {
        std::vector<vector<izenelib::util::UString> > groupPaths;
        split_group_path(propValue, groupPaths);

        try
        {
            for (std::vector<vector<izenelib::util::UString> >::const_iterator pathIt = groupPaths.begin();
                pathIt != groupPaths.end(); ++pathIt)
            {
                PropValueTable::pvid_t pvId = pvTable.insertPropValueId(*pathIt);
                propIdList.push_back(pvId);
            }
        }
        catch (MiningException& e)
        {
            LOG(ERROR) << "exception: " << e.what()
                       << ", doc id: " << docId;
        }
    }

    try
    {
        pvTable.appendPropIdList(propIdList);
    }
    catch (MiningException& e)
    {
        LOG(ERROR) << "exception: " << e.what()
                   << ", doc id: " << docId;
    }
}

void GroupManager::buildDoc_(
        docid_t docId,
        const std::string& propName,
        PropValueTable& pvTable)
{
    std::vector<PropValueTable::pvid_t> propIdList;
    izenelib::util::UString propValue;

    if (documentManager_.getPropertyValue(docId, propName, propValue))
    {
        std::vector<vector<izenelib::util::UString> > groupPaths;
        split_group_path(propValue, groupPaths);

        try
        {
            for (std::vector<vector<izenelib::util::UString> >::const_iterator pathIt = groupPaths.begin();
                pathIt != groupPaths.end(); ++pathIt)
            {
                PropValueTable::pvid_t pvId = pvTable.insertPropValueId(*pathIt);
                propIdList.push_back(pvId);
            }
        }
        catch (MiningException& e)
        {
            LOG(ERROR) << "exception: " << e.what()
                       << ", doc id: " << docId;
        }
    }

    try
    {
        pvTable.appendPropIdList(propIdList);
    }
    catch (MiningException& e)
    {
        LOG(ERROR) << "exception: " << e.what()
                   << ", doc id: " << docId;
    }
}

void GroupManager::buildDoc1_(
        docid_t docId,
        const izenelib::util::UString propValue,
        DateGroupTable& dateTable)
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

            pathIt->front().convertString(dateStr, ENCODING_TYPE);
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
        dateTable.appendDateSet(dateSet);
    }
    catch(MiningException& e)
    {
        LOG(ERROR) << "exception: " << e.what()
                   << ", doc id: " << docId;
    }
}

void GroupManager::buildDoc_(
        docid_t docId,
        const std::string& propName,
        DateGroupTable& dateTable)
{
    DateGroupTable::DateSet dateSet;
    izenelib::util::UString propValue;

    if (documentManager_.getPropertyValue(docId, propName, propValue))
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

            pathIt->front().convertString(dateStr, ENCODING_TYPE);
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
        dateTable.appendDateSet(dateSet);
    }
    catch(MiningException& e)
    {
        LOG(ERROR) << "exception: " << e.what()
                   << ", doc id: " << docId;
    }
}

bool GroupManager::isRebuildProp_(const std::string& propName) const
{
    GroupConfigMap::const_iterator it = groupConfigMap_.find(propName);

    if (it != groupConfigMap_.end())
        return it->second.isRebuild();

    return false;
}
