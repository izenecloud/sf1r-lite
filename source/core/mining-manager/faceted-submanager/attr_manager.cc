#include "attr_manager.h"
#include <mining-manager/util/split_ustr.h>
#include <mining-manager/util/FSUtil.hpp>
#include <document-manager/DocumentManager.h>
#include <mining-manager/MiningException.hpp>

#include <glog/logging.h>

using namespace sf1r::faceted;

AttrManager::AttrManager(
    DocumentManager* documentManager,
    const std::string& dirPath
)
: documentManager_(documentManager)
, dirPath_(dirPath)
{
}

bool AttrManager::open(const AttrConfig& attrConfig)
{
    if (!attrTable_.open(dirPath_, attrConfig.propName))
    {
        LOG(ERROR) << "AttrTable::open() failed, property name: " << attrConfig.propName;
        return false;
    }

    return true;
}

bool AttrManager::processCollection()
{
    LOG(INFO) << "start building attr index data...";

    try
    {
        FSUtil::createDir(dirPath_);
    }
    catch(FileOperationException& e)
    {
        LOG(ERROR) << "exception in FSUtil::createDir: " << e.what();
        return false;
    }

    const char* propName = attrTable_.propName();
    AttrTable::ValueIdTable& idTable = attrTable_.valueIdTable();
    assert(! idTable.empty() && "id 0 should have been reserved in AttrTable constructor");

    const docid_t startDocId = idTable.size();
    const docid_t endDocId = documentManager_->getMaxDocId();
    idTable.reserve(endDocId + 1);

    LOG(INFO) << "start building property: " << propName
        << ", start doc id: " << startDocId
        << ", end doc id: " << endDocId;

    for (docid_t docId = startDocId; docId <= endDocId; ++docId)
    {
        idTable.push_back(AttrTable::ValueIdList());
        AttrTable::ValueIdList& valueIdList = idTable.back();

        Document doc;
        if (documentManager_->getDocument(docId, doc))
        {
            Document::property_iterator it = doc.findProperty(propName);
            if (it != doc.propertyEnd())
            {
                const izenelib::util::UString& propValue = it->second.get<izenelib::util::UString>();
                std::vector<AttrPair> attrPairs;
                split_attr_pair(propValue, attrPairs);

                try
                {
                    for (std::vector<AttrPair>::const_iterator pairIt = attrPairs.begin();
                        pairIt != attrPairs.end(); ++pairIt)
                    {
                        AttrTable::nid_t nameId = attrTable_.nameId(pairIt->first);

                        for (std::vector<izenelib::util::UString>::const_iterator valueIt = pairIt->second.begin();
                            valueIt != pairIt->second.end(); ++valueIt)
                        {
                            AttrTable::vid_t valueId = attrTable_.valueId(nameId, *valueIt);
                            valueIdList.push_back(valueId);
                        }
                    }
                }
                catch(MiningException& e)
                {
                    LOG(ERROR) << "exception: " << e.what()
                        << ", doc id: " << docId;
                }
            }
            else
            {
                LOG(WARNING) << "Document::findProperty, doc id " << docId
                    << " has no value on property " << propName;
            }
        }
        else
        {
            LOG(ERROR) << "DocumentManager::getDocument() failed, doc id: " << docId;
        }

        if (docId % 100000 == 0)
        {
            LOG(INFO) << "inserted doc id: " << docId;
        }
    }

    if (!attrTable_.flush())
    {
        LOG(ERROR) << "AttrTable::flush() failed, property name: " << propName;
    }

    LOG(INFO) << "finished building attr index data";
    return true;
}
