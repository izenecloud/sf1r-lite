#include "attr_table.h"
#include <mining-manager/util/fcontainer.h>
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
const char* SUFFIX_NAME_STR = ".name_str.txt";
const char* SUFFIX_VALUE_STR = ".value_str.txt";
const char* SUFFIX_NAME_ID = ".name_id.txt";
const char* SUFFIX_VALUE_ID = ".value_id.txt";
}

// as id 0 is reserved for empty value
// members are initialized to size 1
AttrTable::AttrTable(
)
: nameStrVec_(1)
, saveNameStrNum_(0)
, valueStrVec_(1)
, saveValueStrNum_(0)
, nameIdVec_(1)
, saveNameIdNum_(0)
, valueIdTable_(1)
, saveDocIdNum_(0)
{
}

AttrTable::nid_t AttrTable::nameId(const izenelib::util::UString& name)
{
    assert(nameStrVec_.size() == valueStrTable_.size());

    nid_t nameId = nameStrVec_.size();

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

AttrTable::vid_t AttrTable::valueId(nid_t nameId, const izenelib::util::UString& value)
{
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

bool AttrTable::open(
    const std::string& dirPath,
    const std::string& propName
)
{
    dirPath_ = dirPath;
    propName_ = propName;

    if (!load_container(dirPath_, propName_ + SUFFIX_NAME_STR, nameStrVec_, saveNameStrNum_) ||
        !load_container(dirPath_, propName_ + SUFFIX_VALUE_STR, valueStrVec_, saveValueStrNum_) ||
        !load_container(dirPath_, propName_ + SUFFIX_NAME_ID, nameIdVec_, saveNameIdNum_) ||
        !load_container(dirPath_, propName_ + SUFFIX_VALUE_ID, valueIdTable_, saveDocIdNum_))
    {
        return false;
    }

    nameStrMap_.clear();
    for (unsigned int i = 0; i < nameStrVec_.size(); ++i)
    {
        nameStrMap_[nameStrVec_[i]] = i;
    }

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
    if (!save_container(dirPath_, propName_ + SUFFIX_NAME_STR, nameStrVec_, saveNameStrNum_) ||
        !save_container(dirPath_, propName_ + SUFFIX_VALUE_STR, valueStrVec_, saveValueStrNum_) ||
        !save_container(dirPath_, propName_ + SUFFIX_NAME_ID, nameIdVec_, saveNameIdNum_) ||
        !save_container(dirPath_, propName_ + SUFFIX_VALUE_ID, valueIdTable_, saveDocIdNum_))
    {
        return false;
    }

    return true;
}
