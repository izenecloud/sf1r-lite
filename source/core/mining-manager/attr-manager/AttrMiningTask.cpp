#include "AttrMiningTask.h"
#include "../util/split_ustr.h"
#include "../util/FSUtil.hpp"
#include "../MiningException.hpp"
#include <document-manager/DocumentManager.h>
#include <glog/logging.h>

NS_FACETED_BEGIN
AttrMiningTask::AttrMiningTask(DocumentManager& documentManager
        , AttrTable& attrTable
        , std::string dirPath
        , const AttrConfig& attrConfig)
    : documentManager_(documentManager)
    , attrTable_(attrTable)
    , dirPath_(dirPath)
    , attrConfig_(attrConfig)
{
}

bool AttrMiningTask::preProcess(int64_t timestamp)
{
    const docid_t startDocId = attrTable_.docIdNum();
    const docid_t endDocId = documentManager_.getMaxDocId();

    if (startDocId > endDocId)
        return false;

    attrTable_.resize(endDocId + 1);
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

bool AttrMiningTask::buildDocument(docid_t docID, const Document& doc)
{
    const std::string propName(attrTable_.propName());
    std::vector<AttrTable::vid_t> valueIdList;
    Document::property_const_iterator it = doc.findProperty(propName);
    if (it != doc.propertyEnd())
    {
        izenelib::util::UString propValue(propstr_to_ustr(it->second.get<PropertyValue::PropertyValueStrType>()));
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
        attrTable_.setValueIdList(docID, valueIdList);
    }
    catch(MiningException& e)
    {
        LOG(ERROR) << "exception: " << e.what()
                   << ", doc id: " << docID;
    }
    return true;
}

NS_FACETED_END
