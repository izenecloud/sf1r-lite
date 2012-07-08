#include "GroupManager.h"
#include "DateStrParser.h"
#include <mining-manager/util/split_ustr.h>
#include <mining-manager/util/FSUtil.hpp>
#include <document-manager/DocumentManager.h>
#include <mining-manager/MiningException.hpp>

#include <iostream>
#include <cassert>

#include <glog/logging.h>

using namespace sf1r::faceted;

namespace
{

const izenelib::util::UString::EncodingType ENCODING_TYPE = izenelib::util::UString::UTF_8;

}

GroupManager::GroupManager(
    DocumentManager* documentManager,
    const std::string& dirPath
)
: documentManager_(documentManager)
, dirPath_(dirPath)
, dateStrParser_(*DateStrParser::get())
{
}

bool GroupManager::open(const std::vector<GroupConfig>& configVec)
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

    for (std::vector<GroupConfig>::const_iterator it = configVec.begin();
        it != configVec.end(); ++it)
    {
        bool result = false;
        const std::string& propName = it->propName;

        switch (it->propType)
        {
        case STRING_PROPERTY_TYPE:
            result = createPropValueTable_(propName);
            break;

        case DATETIME_PROPERTY_TYPE:
            result = createDateGroupTable_(propName);
            break;

        default:
            // ignore other types
            result = true;
            break;
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
        buildStrPropForCollection_(it->second);
    }

    for (DatePropMap::iterator it = datePropMap_.begin();
        it != datePropMap_.end(); ++it)
    {
        buildDatePropForCollection_(it->second);
    }

    LOG(INFO) << "finished building group index data";
    return true;
}

void GroupManager::buildStrPropForCollection_(PropValueTable& pvTable)
{
    const std::string& propName = pvTable.propName();

    const docid_t startDocId = pvTable.docIdNum();
    const docid_t endDocId = documentManager_->getMaxDocId();
    assert(startDocId && "docid 0 should have been reserved in PropValueTable constructor");

    if (startDocId > endDocId)
        return;

    LOG(INFO) << "start building property: " << propName
                << ", start doc id: " << startDocId
                << ", end doc id: " << endDocId;

    pvTable.reserveDocIdNum(endDocId + 1);

    for (docid_t docId = startDocId; docId <= endDocId; ++docId)
    {
        if (docId % 100000 == 0)
        {
            std::cout << "\rinserting doc id: " << docId << "\t" << std::flush;
        }

        buildStrPropForDoc_(docId, propName, pvTable);
    }
    std::cout << "\rinserting doc id: " << endDocId << "\t" << std::endl;

    if (!pvTable.flush())
    {
        LOG(ERROR) << "PropValueTable::flush() failed, property name: " << propName;
    }
}

void GroupManager::buildStrPropForDoc_(
    docid_t docId,
    const std::string& propName,
    PropValueTable& pvTable
)
{
    std::vector<PropValueTable::pvid_t> propIdList;
    Document doc;

    if (documentManager_->getDocument(docId, doc))
    {
        Document::property_iterator it = doc.findProperty(propName);
        if (it != doc.propertyEnd())
        {
            const izenelib::util::UString& propValue = it->second.get<izenelib::util::UString>();
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
            catch(MiningException& e)
            {
                LOG(ERROR) << "exception: " << e.what()
                           << ", doc id: " << docId;
            }
        }
    }

    try
    {
        pvTable.appendPropIdList(propIdList);
    }
    catch(MiningException& e)
    {
        LOG(ERROR) << "exception: " << e.what()
                   << ", doc id: " << docId;
    }
}

void GroupManager::buildDatePropForCollection_(DateGroupTable& dateTable)
{
    const std::string& propName = dateTable.propName();

    const docid_t startDocId = dateTable.docIdNum();
    const docid_t endDocId = documentManager_->getMaxDocId();
    assert(startDocId && "docid 0 should have been reserved in DateGroupTable constructor");

    if (startDocId > endDocId)
        return;

    LOG(INFO) << "start building property: " << propName
                << ", start doc id: " << startDocId
                << ", end doc id: " << endDocId;

    dateTable.reserveDocIdNum(endDocId + 1);

    for (docid_t docId = startDocId; docId <= endDocId; ++docId)
    {
        if (docId % 100000 == 0)
        {
            std::cout << "\rinserting doc id: " << docId << "\t" << std::flush;
        }

        buildDatePropForDoc_(docId, propName, dateTable);
    }
    std::cout << "\rinserting doc id: " << endDocId << "\t" << std::endl;

    if (!dateTable.flush())
    {
        LOG(ERROR) << "DateGroupTable::flush() failed, property name: " << propName;
    }
}

void GroupManager::buildDatePropForDoc_(
    docid_t docId,
    const std::string& propName,
    DateGroupTable& dateTable
)
{
    DateGroupTable::DateSet dateSet;
    Document doc;

    if (documentManager_->getDocument(docId, doc))
    {
        Document::property_iterator it = doc.findProperty(propName);
        if (it != doc.propertyEnd())
        {
            const izenelib::util::UString& propValue = it->second.get<izenelib::util::UString>();
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
