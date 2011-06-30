#include "attr_table.h"
#include <mining-manager/MiningException.hpp>

#include <iostream>
#include <fstream>
#include <cassert>
#include <set>

#include <boost/filesystem.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/archive_exception.hpp>
#include <boost/serialization/vector.hpp>

#include <boost/lexical_cast.hpp>

#include <glog/logging.h>

using namespace sf1r::faceted;

namespace
{

template<class T> bool saveFile(
    const std::string& dirPath,
    const std::string& fileName,
    const T& container,
    unsigned int& count,
    bool binary = false
)
{
    if (count < container.size())
    {
        boost::filesystem::path filePath(dirPath);
        filePath /= fileName;
        std::string pathStr = filePath.string();

        LOG(INFO) << "saving file: " << pathStr
                  << ", element num: " << container.size();

        std::ios_base::openmode openMode = std::ios_base::out;
        if (binary)
        {
            openMode |= std::ios_base::binary;
        }

        std::ofstream ofs(pathStr.c_str(), openMode);
        if (! ofs)
        {
            LOG(ERROR) << "failed opening file " << pathStr;
            return false;
        }

        try
        {
            if (binary)
            {
                boost::archive::binary_oarchive oa(ofs);
                oa << container;
            }
            else
            {
                boost::archive::text_oarchive oa(ofs);
                oa << container;
            }
        }
        catch(boost::archive::archive_exception& e)
        {
            LOG(ERROR) << "exception in boost::archive::text_oarchive or binary_oarchive: " << e.what()
                       << ", pathStr: " << pathStr;
            return false;
        }

        count = container.size();
    }

    return true;
}

template<class T> bool loadFile(
    const std::string& dirPath,
    const std::string& fileName,
    T& container,
    unsigned int& count,
    bool binary = false
)
{
    boost::filesystem::path filePath(dirPath);
    filePath /= fileName;
    std::string pathStr = filePath.string();

    std::ios_base::openmode openMode = std::ios_base::in;
    if (binary)
    {
        openMode |= std::ios_base::binary;
    }

    std::ifstream ifs(pathStr.c_str(), openMode);
    if (ifs)
    {
        LOG(INFO) << "start loading file : " << pathStr;

        try
        {
            if (binary)
            {
                boost::archive::binary_iarchive ia(ifs);
                ia >> container;
            }
            else
            {
                boost::archive::text_iarchive ia(ifs);
                ia >> container;
            }
        }
        catch(boost::archive::archive_exception& e)
        {
            LOG(ERROR) << "exception in boost::archive::text_iarchive or binary_iarchive: " << e.what()
                       << ", pathStr: " << pathStr;
            return false;
        }

        count = container.size();

        LOG(INFO) << "finished loading , element num: " << count;
    }

    return true;
}

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

    if (!loadFile(dirPath_, propName_ + ".name.map", nameStrVec_, saveNameStrNum_)
        || !loadFile(dirPath_, propName_ + ".value.map", valueStrVec_, saveValueStrNum_)
        || !loadFile(dirPath_, propName_ + ".name.table", nameIdVec_, saveNameIdNum_, true)
        || !loadFile(dirPath_, propName_ + ".value.table", valueIdTable_, saveDocIdNum_, true))
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
    if (!saveFile(dirPath_, propName_ + ".name.map", nameStrVec_, saveNameStrNum_)
        || !saveFile(dirPath_, propName_ + ".value.map", valueStrVec_, saveValueStrNum_)
        || !saveFile(dirPath_, propName_ + ".name.table", nameIdVec_, saveNameIdNum_, true)
        || !saveFile(dirPath_, propName_ + ".value.table", valueIdTable_, saveDocIdNum_, true))
    {
        return false;
    }

    return true;
}
