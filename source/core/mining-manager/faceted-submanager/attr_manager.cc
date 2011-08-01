#include "attr_manager.h"
#include <mining-manager/util/FSUtil.hpp>
#include <document-manager/DocumentManager.h>
#include <mining-manager/MiningException.hpp>

#include <glog/logging.h>

using namespace sf1r::faceted;

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
