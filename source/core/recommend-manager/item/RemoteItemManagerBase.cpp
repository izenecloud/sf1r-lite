#include "RemoteItemManagerBase.h"
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

bool RemoteItemManagerBase::getItem(
    itemid_t itemId,
    const std::vector<std::string>& propList,
    Document& doc
)
{
    GetDocumentsByIdsActionItem request;
    RawTextResultFromSIA response;

    if (createRequest_(itemId, propList, request) &&
        sendRequest_(request, response) &&
        getDocFromResponse_(response, propList, doc))
    {
        doc.setId(itemId);
        return true;
    }

    return false;
}

bool RemoteItemManagerBase::hasItem(itemid_t itemId)
{
    std::vector<std::string> propList;
    propList.push_back(DOCID);

    Document doc;
    return getItem(itemId, propList, doc);
}

bool RemoteItemManagerBase::createRequest_(
    itemid_t itemId,
    const std::vector<std::string>& propList,
    GetDocumentsByIdsActionItem& request
)
{
    std::string strItemId;
    if (! itemIdGenerator_->itemIdToStrId(itemId, strItemId))
        return false;

    request.collectionName_ = collection_;
    request.env_.encodingType_ = ENCODING_TYPE_STR;
    request.docIdList_.push_back(strItemId);

    for (std::vector<std::string>::const_iterator propIt = propList.begin();
        propIt != propList.end(); ++propIt)
    {
        request.displayPropertyList_.push_back(DisplayProperty(*propIt));
    }

    return true;
}

bool RemoteItemManagerBase::getDocFromResponse_(
    const RawTextResultFromSIA& response,
    const std::vector<std::string>& propList,
    Document& doc
) const
{
    typedef std::vector<izenelib::util::UString> DocValues;
    typedef std::vector<DocValues> PropDocTable;
    const PropDocTable& propDocTable = response.fullTextOfDocumentInPage_;

    if (propDocTable.size() != propList.size())
    {
        LOG(ERROR) << "invalid property count in RawTextResultFromSIA::fullTextOfDocumentInPage_";
        return false;
    }

    doc.clear();

    PropDocTable::const_iterator tableIt = propDocTable.begin();
    for (std::vector<std::string>::const_iterator propIt = propList.begin();
        propIt != propList.end(); ++propIt, ++tableIt)
    {
        const DocValues& docValues = *tableIt;
        if (! docValues.empty())
        {
            doc.property(*propIt) = docValues[0];
        }
    }

    return true;
}

} // namespace sf1r
