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
    , propName_(attrTable.propName())
    , swapper_(attrTable)
    , dirPath_(dirPath)
    , attrConfig_(attrConfig)
{
}

bool AttrMiningTask::preProcess(int64_t timestamp)
{
    const docid_t startDocId = swapper_.reader.docIdNum();
    const docid_t endDocId = documentManager_.getMaxDocId();

    if (startDocId > endDocId)
        return false;

    swapper_.copy(endDocId + 1);
    return true;
}

docid_t AttrMiningTask::getLastDocId()
{
    return swapper_.reader.docIdNum();
}

bool AttrMiningTask::postProcess()
{
    if (!swapper_.writer.flush())
    {
        LOG(ERROR) << "AttrTable::flush() failed, property name: " << propName_;
        return false;
    }

    swapper_.swap();

    LOG(INFO) << "finished building attr index data";
    return true;
}

bool AttrMiningTask::buildDocument(docid_t docID, const Document& doc)
{
    std::vector<AttrTable::vid_t> valueIdList;
    Document::property_const_iterator it = doc.findProperty(propName_);
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

                AttrTable::nid_t nameId =
                        swapper_.writer.insertNameId(attrName);

                for (std::vector<izenelib::util::UString>::const_iterator valueIt = pairIt->second.begin();
                    valueIt != pairIt->second.end(); ++valueIt)
                {
                    AttrTable::vid_t valueId =
                            swapper_.writer.insertValueId(nameId, *valueIt);
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
        swapper_.writer.setValueIdList(docID, valueIdList);
    }
    catch(MiningException& e)
    {
        LOG(ERROR) << "exception: " << e.what()
                   << ", doc id: " << docID;
    }
    return true;
}

NS_FACETED_END
