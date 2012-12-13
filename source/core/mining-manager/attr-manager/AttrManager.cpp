#include "AttrManager.h"
#include "../util/split_ustr.h"
#include "../util/FSUtil.hpp"
#include "../MiningException.hpp"
#include <document-manager/DocumentManager.h>

#include <glog/logging.h>
#include <iostream>

using namespace sf1r::faceted;

AttrManager::AttrManager(
        const AttrConfig& attrConfig,
        const std::string& dirPath,
        DocumentManager& documentManager)
    : attrConfig_(attrConfig)
    , dirPath_(dirPath)
    , documentManager_(documentManager)
    , attrMiningTask_(NULL)
{
}

bool AttrManager::open()
{
    LOG(INFO) << "Start loading attr directory: " << dirPath_;

    if (!attrTable_.open(dirPath_, attrConfig_.propName))
    {
        LOG(ERROR) << "AttrTable::open() failed, property name: " << attrConfig_.propName;
        return false;
    }

    LOG(INFO) << "End attr loading";
    attrMiningTask_ = new AttrMiningTask(documentManager_, attrTable_, dirPath_, attrConfig_);

    if(attrMiningTask_ == NULL)
    {
        LOG(INFO)<<"Build AttrMiningTask ERROR"<<endl;
        return false;
    }

    try
    {
        FSUtil::createDir(dirPath_);//xxx
    }
    catch(FileOperationException& e)
    {
        LOG(ERROR) << "exception in FSUtil::createDir: " << e.what();
        return false;
    }
    return true;
}

bool AttrManager::processCollection()
{
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
    const docid_t startDocId = attrTable_.docIdNum();
    const docid_t endDocId = documentManager_.getMaxDocId();
    assert(startDocId && "id 0 should have been reserved in AttrTable constructor");

    if (startDocId > endDocId)
        return true;

    LOG(INFO) << "start building attr index data for property: " << propName
              << ", start doc id: " << startDocId
              << ", end doc id: " << endDocId;

    attrTable_.reserveDocIdNum(endDocId + 1);

    for (docid_t docId = startDocId; docId <= endDocId; ++docId)
    {
        if (docId % 100000 == 0)
        {
            std::cout << "\rinserting doc id: " << docId << "\t" << std::flush;
        }

        buildDoc_(docId, propName);
    }
    std::cout << "\rinserting doc id: " << endDocId << "\t" << std::endl;

    if (!attrTable_.flush())
    {
        LOG(ERROR) << "AttrTable::flush() failed, property name: " << propName;
    }

    LOG(INFO) << "finished building attr index data";
    return true;
}

void AttrManager::buildDoc_(docid_t docId, const std::string& propName)
{
    std::vector<AttrTable::vid_t> valueIdList;
    Document doc;

    if (documentManager_.getDocument(docId, doc))
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
                    const izenelib::util::UString& attrName = pairIt->first;

                    if (attrConfig_.isExcludeAttrName(attrName))
                        continue;

                    AttrTable::nid_t nameId = attrTable_.insertNameId(attrName);

                    for (std::vector<izenelib::util::UString>::const_iterator valueIt = pairIt->second.begin();
                        valueIt != pairIt->second.end(); ++valueIt)
                    {
                        AttrTable::vid_t valueId = attrTable_.insertValueId(nameId, *valueIt);
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
    }

    try
    {
        attrTable_.appendValueIdList(valueIdList);
    }
    catch(MiningException& e)
    {
        LOG(ERROR) << "exception: " << e.what()
                   << ", doc id: " << docId;
    }
}
