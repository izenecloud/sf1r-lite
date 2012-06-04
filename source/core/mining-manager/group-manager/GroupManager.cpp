#include "GroupManager.h"
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
{
}

bool GroupManager::open(const std::vector<GroupConfig>& configVec)
{
    propValueMap_.clear();

    LOG(INFO) << "Start loading group directory: " << dirPath_;

    for (std::vector<GroupConfig>::const_iterator it = configVec.begin();
        it != configVec.end(); ++it)
    {
        if (! it->isStringType())
            continue;

        std::pair<PropValueMap::iterator, bool> res =
            propValueMap_.insert(PropValueMap::value_type(it->propName, PropValueTable(dirPath_, it->propName)));

        if (res.second)
        {
            PropValueTable& pvTable = res.first->second;
            if (!pvTable.open())
            {
                LOG(ERROR) << "PropValueTable::open() failed, property name: " << it->propName;
                return false;
            }
        }
        else
        {
            LOG(WARNING) << "the group property " << it->propName << " is opened already.";
        }
    }

    LOG(INFO) << "End group loading";

    return true;
}

bool GroupManager::processCollection()
{
    LOG(INFO) << "start building group index data...";

    try
    {
        FSUtil::createDir(dirPath_);
    }
    catch(FileOperationException& e)
    {
        LOG(ERROR) << "exception in FSUtil::createDir: " << e.what();
        return false;
    }

    for (PropValueMap::iterator it = propValueMap_.begin();
        it != propValueMap_.end(); ++it)
    {
        const std::string& propName = it->first;
        PropValueTable& pvTable = it->second;

        const docid_t startDocId = pvTable.docIdNum();
        const docid_t endDocId = documentManager_->getMaxDocId();
        assert(startDocId && "docid 0 should have been reserved in PropValueTable constructor");

        if (startDocId > endDocId)
            continue;

        LOG(INFO) << "start building property: " << propName
                  << ", start doc id: " << startDocId
                  << ", end doc id: " << endDocId;

        pvTable.reserveDocIdNum(endDocId + 1);
        std::vector<PropValueTable::pvid_t> propIdList;

        for (docid_t docId = startDocId; docId <= endDocId; ++docId)
        {
            if (docId % 100000 == 0)
            {
                std::cout << "\rinserting doc id: " << docId << "\t" << std::flush;
            }

            propIdList.clear();

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
            pvTable.appendPropIdList(propIdList);
        }
        std::cout << "\rinserting doc id: " << endDocId << "\t" << std::endl;

        if (!pvTable.flush())
        {
            LOG(ERROR) << "PropValueTable::flush() failed, property name: " << propName;
        }
    }

    LOG(INFO) << "finished building group index data";
    return true;
}

