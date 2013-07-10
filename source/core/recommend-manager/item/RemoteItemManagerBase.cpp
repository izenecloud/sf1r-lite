#include "RemoteItemManagerBase.h"
#include "ItemContainer.h"
#include "ItemIdGenerator.h"
#include <document-manager/Document.h>
#include <query-manager/ActionItem.h>
#include <common/ResultType.h>

#include <glog/logging.h>

namespace
{
const std::string DOCID("DOCID");
const std::string ENCODING_TYPE_STR("UTF-8");
}

namespace sf1r
{

RemoteItemManagerBase::RemoteItemManagerBase(
    const std::string& collection,
    ItemIdGenerator* itemIdGenerator
)
    : collection_(collection)
    , itemIdGenerator_(itemIdGenerator)
{
}

bool RemoteItemManagerBase::hasItem(itemid_t itemId)
{
    std::vector<std::string> propList;
    propList.push_back(DOCID);

    Document doc(itemId);
    SingleItemContainer itemContainer(doc);

    return getItemPropsImpl_(propList, itemContainer);
}

bool RemoteItemManagerBase::getItemProps(
    const std::vector<std::string>& propList,
    ItemContainer& itemContainer
)
{
    if (propList.empty() || itemContainer.getItemNum() == 0)
        return true;

    if (propList.size() == 1 && propList[0] == DOCID)
        return getItemStrIds_(itemContainer);

    return getItemPropsImpl_(propList, itemContainer);
}

bool RemoteItemManagerBase::getItemStrIds_(ItemContainer& itemContainer)
{
    const std::size_t itemNum = itemContainer.getItemNum();

    for (std::size_t i = 0; i < itemNum; ++i)
    {
        Document& item = itemContainer.getItem(i);
        std::string strItemId;

        if (! itemIdGenerator_->itemIdToStrId(item.getId(), strItemId))
            return false;

        item.property(DOCID) = str_to_propstr(strItemId);
    }

    return true;
}

bool RemoteItemManagerBase::getItemPropsImpl_(
    const std::vector<std::string>& propList,
    ItemContainer& itemContainer
)
{
    GetDocumentsByIdsActionItem request;
    RawTextResultFromSIA response;

    return createRequest_(propList, itemContainer, request) &&
           sendRequest_(request, response) &&
           getItemsFromResponse_(propList, response, itemContainer);
}

bool RemoteItemManagerBase::createRequest_(
    const std::vector<std::string>& propList,
    ItemContainer& itemContainer,
    GetDocumentsByIdsActionItem& request
)
{
    request.collectionName_ = collection_;
    request.env_.encodingType_ = ENCODING_TYPE_STR;

    for (std::vector<std::string>::const_iterator propIt = propList.begin();
        propIt != propList.end(); ++propIt)
    {
        request.displayPropertyList_.push_back(DisplayProperty(*propIt));
    }

    const std::size_t itemNum = itemContainer.getItemNum();

    for (std::size_t i = 0; i < itemNum; ++i)
    {
        Document& item = itemContainer.getItem(i);
        item.clearProperties();
        std::string strItemId;

        if (! itemIdGenerator_->itemIdToStrId(item.getId(), strItemId))
            return false;

        request.docIdList_.push_back(strItemId);
    }

    return true;
}

bool RemoteItemManagerBase::getItemsFromResponse_(
    const std::vector<std::string>& propList,
    const RawTextResultFromSIA& response,
    ItemContainer& itemContainer
) const
{
    typedef std::vector<izenelib::util::UString> DocValues;
    typedef std::vector<DocValues> PropDocTable;

    const PropDocTable& propDocTable = response.fullTextOfDocumentInPage_;
    const std::size_t propNum = propList.size();
    if (propDocTable.size() != propNum)
    {
        LOG(ERROR) << "invalid property count in RawTextResultFromSIA::fullTextOfDocumentInPage_"
                   << ", request count: " << propNum
                   << ", returned count: " << propDocTable.size();
        return false;
    }

    const std::size_t itemNum = itemContainer.getItemNum();
    for (std::size_t propId = 0; propId < propNum; ++propId)
    {
        const DocValues& docValues = propDocTable[propId];
        const std::string& propName = propList[propId];

        if (docValues.size() != itemNum)
        {
            LOG(ERROR) << "invalid doc count in RawTextResultFromSIA::fullTextOfDocumentInPage_"
                       << ", request count: " << itemNum
                       << ", returned count: " << docValues.size();
            return false;
        }

        for (std::size_t docValueId = 0; docValueId < itemNum; ++docValueId)
        {
            const izenelib::util::UString& propValue = docValues[docValueId];

            if (! propValue.empty())
            {
                Document& doc = itemContainer.getItem(docValueId);
                doc.property(propName) = ustr_to_propstr(propValue);
            }
        }
    }

    for (std::size_t i = 0; i < itemNum; ++i)
    {
        if (itemContainer.getItem(i).isEmpty())
            return false;
    }

    return true;
}

} // namespace sf1r
