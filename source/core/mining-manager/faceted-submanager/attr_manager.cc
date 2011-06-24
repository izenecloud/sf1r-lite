#include "attr_manager.h"
#include "group_label.h"
#include <mining-manager/util/FSUtil.hpp>
#include <document-manager/DocumentManager.h>
#include <mining-manager/MiningException.hpp>

#include <iostream>

#include <glog/logging.h>

using namespace sf1r::faceted;

#define DEBUG_PRINT_GROUP 0

namespace
{

const izenelib::util::UString::EncodingType ENCODING_TYPE = izenelib::util::UString::UTF_8;

/** colon to split attribute name and value */
const izenelib::util::UString COLON(":", ENCODING_TYPE);

/** vertical line to split attribute values */
const izenelib::util::UString VL("|", ENCODING_TYPE);

/** comma to split pairs of attribute name and value */
const izenelib::util::UString COMMA_VL(",|", ENCODING_TYPE);

/** quote to embed escape characters */
const izenelib::util::UString QUOTE("\"", ENCODING_TYPE);

/** types used in @c getGroupRep() */
typedef std::vector<docid_t> DocIdList;
typedef std::map<AttrTable::vid_t, DocIdList> DocIdMap;
struct AttrNameResult
{
    AttrNameResult(): docCount_(0) {}
    int docCount_;
    DocIdMap docIdMap_;
};
typedef std::map<AttrTable::nid_t, AttrNameResult> AttrGroupResult;

/**
 * scans from @p first to @p last until any character in @p delimStr is reached,
 * copy the processed string into @p output.
 * @param first the begin position
 * @param last the end position
 * @param delimStr if any character in @p delimStr is reached, stop the scanning
 * @param output store the sub string
 * @return the last position scaned
 */
template<class InputIterator, class T>
InputIterator parseAttrStr(
    InputIterator first,
    InputIterator last,
    const T& delimStr,
    T& output
)
{
    output.clear();

    InputIterator it = first;

    if (first != last && *first == QUOTE[0])
    {
        // escape start quote
        ++it;

        for (; it != last; ++it)
        {
            // candidate end quote
            if (*it == QUOTE[0])
            {
                ++it;

                // convert double quotes to single quote
                if (it != last && *it == QUOTE[0])
                {
                    output.push_back(*it);
                }
                else
                {
                    // end quote escaped
                    break;
                }
            }
            else
            {
                output.push_back(*it);
            }
        }

        // escape all characters between end quote and delimStr
        for (; it != last; ++it)
        {
            if (delimStr.find(*it) != T::npos)
                break;
        }
    }
    else
    {
        for (; it != last; ++it)
        {
            if (delimStr.find(*it) != T::npos)
                break;

            output.push_back(*it);
        }

        // as no quote "" is surrounded,
        // trim space characters at head and tail
        izenelib::util::UString::algo::compact(output);
    }

    return it;
}

}

void AttrManager::splitAttrPair(
    const izenelib::util::UString& src,
    std::vector<AttrPair>& attrPairs
)
{
    izenelib::util::UString::const_iterator it = src.begin();
    izenelib::util::UString::const_iterator endIt = src.end();

    while (it != endIt)
    {
        izenelib::util::UString name;
        it = parseAttrStr(it, endIt, COLON, name);
        if (it == endIt || name.empty())
            break;

        AttrPair attrPair;
        attrPair.first = name;
        izenelib::util::UString value;

        // escape colon
        ++it;
        it = parseAttrStr(it, endIt, COMMA_VL, value);
        if (!value.empty())
        {
            attrPair.second.push_back(value);
        }

        while (it != endIt && *it == VL[0])
        {
            // escape vertical line
            ++it;
            it = parseAttrStr(it, endIt, COMMA_VL, value);
            if (!value.empty())
            {
                attrPair.second.push_back(value);
            }
        }
        if (!attrPair.second.empty())
        {
            attrPairs.push_back(attrPair);
        }

        if (it != endIt)
        {
            assert(*it == COMMA_VL[0]);
            // escape comma
            ++it;
        }
    }
}

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
                splitAttrPair(propValue, attrPairs);

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

bool AttrManager::getGroupRep(
    const std::vector<unsigned int>& docIdList,
    const std::vector<std::pair<std::string, std::string> >& attrLabelList,
    const GroupLabel* groupLabel,
    faceted::OntologyRep& groupRep
)
{
    // a condition pair of attribute name id and attribute value id
    typedef std::pair<AttrTable::nid_t, AttrTable::vid_t> CondPair;
    typedef std::vector<CondPair> CondList;
    CondList condList;
    const AttrTable& constAttrTable = attrTable_;
    for (std::size_t i = 0; i < attrLabelList.size(); ++i)
    {
        izenelib::util::UString nameUStr(attrLabelList[i].first, ENCODING_TYPE);
        izenelib::util::UString valueUStr(attrLabelList[i].second, ENCODING_TYPE);

        AttrTable::nid_t nameId = constAttrTable.nameId(nameUStr);
        AttrTable::vid_t valueId = constAttrTable.valueId(nameId, valueUStr);
        condList.push_back(CondPair(nameId, valueId));
    }

    AttrGroupResult groupResult;

    typedef std::set<AttrTable::vid_t> ValueIdSet;
    typedef std::map<AttrTable::nid_t, ValueIdSet> NameIdMap;

    const AttrTable::ValueIdTable& valueIdTable = constAttrTable.valueIdTable();

    for (std::vector<unsigned int>::const_iterator docIt = docIdList.begin();
        docIt != docIdList.end(); ++docIt)
    {
        docid_t docId = *docIt;

        // filter out doc not belongs to selected group label
        if (groupLabel && !groupLabel->checkDoc(docId))
            continue;

        // this doc has not built attribute index data
        if (docId >= valueIdTable.size())
            continue;

        NameIdMap nameIdMap;
        const AttrTable::ValueIdList& valueIdList = valueIdTable[docId];
        for (AttrTable::ValueIdList::const_iterator valueIt = valueIdList.begin();
            valueIt != valueIdList.end(); ++valueIt)
        {
            AttrTable::nid_t nameId = constAttrTable.valueId2NameId(*valueIt);
            nameIdMap[nameId].insert(*valueIt);
        }

        for (NameIdMap::const_iterator nameIt = nameIdMap.begin();
            nameIt != nameIdMap.end(); ++nameIt)
        {
            AttrTable::nid_t nameId = nameIt->first;
            const ValueIdSet& valueIdSet = nameIt->second;

            bool isMeetCond = true;
            for (CondList::const_iterator condIt = condList.begin();
                condIt != condList.end(); ++condIt)
            {
                if (nameId != condIt->first
                    && valueIdSet.find(condIt->second) == valueIdSet.end())
                {
                    isMeetCond = false;
                    break;
                }
            }

            if (isMeetCond)
            {
                AttrNameResult& nameResult = groupResult[nameId];
                ++nameResult.docCount_;

                for (ValueIdSet::const_iterator setIt = valueIdSet.begin();
                    setIt != valueIdSet.end(); ++setIt)
                {
                    nameResult.docIdMap_[*setIt].push_back(docId);
                }
            }
        }
    }

    std::list<sf1r::faceted::OntologyRepItem>& itemList = groupRep.item_list;
    for (AttrGroupResult::iterator groupIt = groupResult.begin();
        groupIt != groupResult.end(); ++groupIt)
    {
        // attribute name as root node
        itemList.push_back(sf1r::faceted::OntologyRepItem());
        sf1r::faceted::OntologyRepItem& nameItem = itemList.back();
        nameItem.text = constAttrTable.nameStr(groupIt->first);
        nameItem.doc_count = groupIt->second.docCount_;

        // attribute values are appended as level 1
        DocIdMap& docIdMap = groupIt->second.docIdMap_;
        for (DocIdMap::iterator mapIt = docIdMap.begin();
            mapIt != docIdMap.end(); ++mapIt)
        {
            itemList.push_back(sf1r::faceted::OntologyRepItem());
            sf1r::faceted::OntologyRepItem& valueItem = itemList.back();
            valueItem.level = 1;
            valueItem.text = constAttrTable.valueStr(mapIt->first);
            valueItem.doc_id_list.swap(mapIt->second);
            valueItem.doc_count = valueItem.doc_id_list.size();
        }
    }

    return true;
}
