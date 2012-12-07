#include "AttrMiningTask.h"
#include "../util/split_ustr.h"
#include "../util/FSUtil.hpp"
#include "../MiningException.hpp"
#include <document-manager/DocumentManager.h>
#include "AttrManager.h"
#include <glog/logging.h>
#include <iostream>

NS_FACETED_BEGIN
AttrMiningTask::AttrMiningTask(DocumentManager& documentManager
        , AttrTable& attrTable
        , std::string dirPath
        , const AttrConfig& attrConfig)
    :documentManager_(documentManager)
    , attrTable_(attrTable)
    , dirPath_(dirPath)
    , attrConfig_(attrConfig)
{

}

bool AttrMiningTask::processCollection_forTest()
{
    preProcess();
    docid_t MaxDocid = documentManager_.getMaxDocId();
    const docid_t startDocId = attrTable_.docIdNum();
    Document doc;
    
    for (uint32_t docid = startDocId; docid <= MaxDocid; ++docid)
    {
        if (docid % 10000 == 0)
        {
            std::cout << "\rinserting doc id: " << docid << "\t" << std::flush;
        }
        documentManager_.getDocument(docid, doc);
        buildDocment(docid, doc);
    }
    postProcess();
    return true;
}

bool AttrMiningTask::preProcess()
{
    const docid_t endDocId = documentManager_.getMaxDocId();
    attrTable_.reserveDocIdNum(endDocId + 1);
    return true;
}

docid_t AttrMiningTask::getLastDocId()
{
    return attrTable_.docIdNum();
}

bool AttrMiningTask::postProcess()
{
    const char* propName = attrTable_.propName();  

    if (!attrTable_.flush())
    {
        LOG(ERROR) << "AttrTable::flush() failed, property name: " << propName;
        return false;
    }

    LOG(INFO) << "finished building attr index data";
    return true;
}

bool AttrMiningTask::buildDocment(docid_t docID, const Document& doc)
{
    if (doc.getId() == 0)
        return true;
    const std::string propName(attrTable_.propName());
    std::vector<AttrTable::vid_t> valueIdList;
    Document::property_const_iterator it = doc.findProperty(propName);
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
                       << ", doc id: " << docID;
        }
    }

    try
    {
        attrTable_.appendValueIdList(valueIdList);
    }
    catch(MiningException& e)
    {
        LOG(ERROR) << "exception: " << e.what()
                   << ", doc id: " << docID;
    }
    return true;
}

NS_FACETED_END
