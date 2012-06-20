#include "AttrTable.h"
#include <mining-manager/util/fcontainer_febird.h>
#include <mining-manager/MiningException.hpp>

#include <iostream>
#include <fstream>
#include <cassert>
#include <set>

#include <boost/lexical_cast.hpp>

#include <glog/logging.h>

using namespace sf1r::faceted;

namespace
{
const char* SUFFIX_NAME_STR = ".name_str.bin";
const char* SUFFIX_VALUE_STR = ".value_str.bin";
const char* SUFFIX_NAME_ID = ".name_id.bin";
const char* SUFFIX_INDEX_ID = ".index_id.bin";
const char* SUFFIX_VALUE_ID = ".value_id.bin";
const char* SUFFIX_NAME_VALUE = ".name_value.txt";

const izenelib::util::UString::EncodingType ENCODING_TYPE =
    izenelib::util::UString::UTF_8;
}

// as id 0 is reserved for empty value
// members are initialized to size 1
AttrTable::AttrTable()
    : nameStrVec_(1)
    , saveNameStrNum_(0)
    , valueStrVec_(1)
    , saveValueStrNum_(0)
    , nameIdVec_(1)
    , saveNameIdNum_(0)
    , saveIndexNum_(0)
    , saveValueNum_(0)
{
}

bool AttrTable::open(
    const std::string& dirPath,
    const std::string& propName
)
{
    ScopedWriteLock lock(mutex_);

    dirPath_ = dirPath;
    propName_ = propName;

    if (!load_container_febird(dirPath_, propName_ + SUFFIX_NAME_STR, nameStrVec_, saveNameStrNum_) ||
        !load_container_febird(dirPath_, propName_ + SUFFIX_VALUE_STR, valueStrVec_, saveValueStrNum_) ||
        !load_container_febird(dirPath_, propName_ + SUFFIX_NAME_ID, nameIdVec_, saveNameIdNum_) ||
        !load_container_febird(dirPath_, propName_ + SUFFIX_INDEX_ID, valueIdTable_.indexTable_, saveIndexNum_) ||
        !load_container_febird(dirPath_, propName_ + SUFFIX_VALUE_ID, valueIdTable_.multiValueTable_, saveValueNum_))
    {
        return false;
    }

    LOG(INFO) << "loading " << nameStrVec_.size()
              << " attribute names into map";
    nameStrMap_.clear();
    for (unsigned int i = 0; i < nameStrVec_.size(); ++i)
    {
        nameStrMap_[nameStrVec_[i]] = i;
    }

    LOG(INFO) << "loading " << valueStrVec_.size()
              << " attribute values into map";
    assert(valueStrVec_.size() == nameIdVec_.size());
    valueStrTable_.clear();
    valueStrTable_.resize(nameStrVec_.size());
    for (unsigned int i = 0; i < valueStrVec_.size(); ++i)
    {
        nid_t nameId = nameIdVec_[i];
        valueStrTable_[nameId][valueStrVec_[i]] = i;
    }

    return true;
}

bool AttrTable::flush()
{
    ScopedWriteLock lock(mutex_);

    if (!saveNameValuePair_(dirPath_, propName_ + SUFFIX_NAME_VALUE) ||
        !save_container_febird(dirPath_, propName_ + SUFFIX_NAME_STR, nameStrVec_, saveNameStrNum_) ||
        !save_container_febird(dirPath_, propName_ + SUFFIX_VALUE_STR, valueStrVec_, saveValueStrNum_) ||
        !save_container_febird(dirPath_, propName_ + SUFFIX_NAME_ID, nameIdVec_, saveNameIdNum_) ||
        !save_container_febird(dirPath_, propName_ + SUFFIX_INDEX_ID, valueIdTable_.indexTable_, saveIndexNum_) ||
        !save_container_febird(dirPath_, propName_ + SUFFIX_VALUE_ID, valueIdTable_.multiValueTable_, saveValueNum_))
    {
        return false;
    }

    return true;
}

void AttrTable::reserveDocIdNum(std::size_t num)
{
    ScopedWriteLock lock(mutex_);

    valueIdTable_.indexTable_.reserve(num);
}

void AttrTable::appendValueIdList(const std::vector<vid_t>& inputIdList)
{
    ScopedWriteLock lock(mutex_);

    valueIdTable_.appendIdList(inputIdList);
}

AttrTable::nid_t AttrTable::insertNameId(const izenelib::util::UString& name)
{
    ScopedWriteLock lock(mutex_);

    nid_t nameId = nameStrVec_.size();
    assert(nameId == valueStrTable_.size());

    typedef std::map<izenelib::util::UString, nid_t> NameStrMap;
    std::pair<NameStrMap::iterator, bool> result = nameStrMap_.insert(
        NameStrMap::value_type(name, nameId)
    );

    if (result.second)
    {
        if (nameId != 0)
        {
            nameStrVec_.push_back(name);
            valueStrTable_.push_back(ValueStrMap());
        }
        else
        {
            // overflow
            throw MiningException(
                "attribute name count is out of range",
                boost::lexical_cast<std::string>(nameStrVec_.size()),
                "AttrTable::nameId"
            );
        }
    }
    else
    {
        nameId = result.first->second;
    }

    return nameId;
}

AttrTable::nid_t AttrTable::nameId(const izenelib::util::UString& name) const
{
    ScopedReadLock lock(mutex_);

    nid_t nameId = 0;

    std::map<izenelib::util::UString, nid_t>::const_iterator it = nameStrMap_.find(name);
    if (it != nameStrMap_.end())
    {
        nameId = it->second; 
    }

    return nameId;
}

AttrTable::vid_t AttrTable::insertValueId(nid_t nameId, const izenelib::util::UString& value)
{
    ScopedWriteLock lock(mutex_);

    assert(nameId < valueStrTable_.size());
    assert(valueStrVec_.size() == nameIdVec_.size());

    vid_t valueId = valueStrVec_.size();
    ValueStrMap& valueStrMap = valueStrTable_[nameId];

    std::pair<ValueStrMap::iterator, bool> result = valueStrMap.insert(
        ValueStrMap::value_type(value, valueId)
    );

    if (result.second)
    {
        if (valueId != 0)
        {
            valueStrVec_.push_back(value);
            nameIdVec_.push_back(nameId);
        }
        else
        {
            // overflow
            throw MiningException(
                "attribute value count is out of range",
                boost::lexical_cast<std::string>(valueStrVec_.size()),
                "AttrTable::valueId"
            );
        }
    }
    else
    {
        valueId = result.first->second;
    }

    return valueId;
}

AttrTable::vid_t AttrTable::valueId(nid_t nameId, const izenelib::util::UString& value) const
{
    ScopedReadLock lock(mutex_);

    assert(nameId < valueStrTable_.size());
    vid_t valueId = 0;
    const ValueStrMap& valueStrMap = valueStrTable_[nameId];

    ValueStrMap::const_iterator it = valueStrMap.find(value);
    if (it != valueStrMap.end())
    {
        valueId = it->second;
    }

    return valueId;
}

bool AttrTable::saveNameValuePair_(const std::string& dirPath, const std::string& fileName) const
{
    const unsigned int nameNum = valueStrTable_.size();
    if (nameNum != nameStrVec_.size())
    {
        LOG(ERROR) << "unequal name number in valueStrTable_ and nameStrVec_";
        return false;
    }

    const unsigned int valueNum = valueStrVec_.size();
    if (saveNameStrNum_ >= nameNum && saveValueStrNum_ >= valueNum)
        return true;

    boost::filesystem::path filePath(dirPath);
    filePath /= fileName;
    std::string pathStr = filePath.string();

    LOG(INFO) << "saving file: " << fileName
              << ", name num: " << nameNum
              << ", value num: " << valueNum;

    std::ofstream ofs(pathStr.c_str());
    if (! ofs)
    {
        LOG(ERROR) << "failed opening file " << fileName;
        return false;
    }

    std::string nameStr;
    std::string valueStr;
    for (unsigned int nameId = 1; nameId < nameNum; ++nameId)
    {
        // columns: name id, name str
        nameStrVec_[nameId].convertString(nameStr, ENCODING_TYPE);
        ofs << nameId << "  " << nameStr << std::endl;

        const ValueStrMap& valueStrMap = valueStrTable_[nameId];
        for (ValueStrMap::const_iterator it = valueStrMap.begin();
            it != valueStrMap.end(); ++it)
        {
            it->first.convertString(valueStr, ENCODING_TYPE);

            // columns: value str, value id
            ofs << "    " << valueStr << "  " << it->second << std::endl;
        }
    }

    return true;
}
