#include "GroupManager.h"
#include "DateStrParser.h"
#include "GroupMiningTask.h"
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
GroupManager::~GroupManager()
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
        MiningTask* miningTask = new GroupMiningTask<PropValueTable>(documentManager_, groupConfigMap_, pvTable);
        groupMiningTaskList_.push_back(miningTask);
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
        MiningTask* miningTask = new GroupMiningTask<DateGroupTable>(documentManager_, groupConfigMap_, dateTable);
        groupMiningTaskList_.push_back(miningTask);
    }
    else
    {
        LOG(WARNING) << "the group property " << propName << " is opened already.";
    }
    return true;
}

bool GroupManager::isRebuildProp_(const std::string& propName) const
{
    GroupConfigMap::const_iterator it = groupConfigMap_.find(propName);

    if (it != groupConfigMap_.end())
        return it->second.isRebuild();

    return false;
}
