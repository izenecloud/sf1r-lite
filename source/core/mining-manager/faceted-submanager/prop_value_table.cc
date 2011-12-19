#include "prop_value_table.h"
#include <mining-manager/util/fcontainer.h>
#include <mining-manager/MiningException.hpp>

#include <iostream>
#include <fstream>
#include <cassert>
#include <set>
#include <algorithm> // reverse

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include <glog/logging.h>

namespace
{
const char* SUFFIX_PROP_STR = ".prop_str.txt";
const char* SUFFIX_PARENT_ID = ".parent_id.txt";
const char* SUFFIX_VALUE_ID = ".value_id.txt";
const char* SUFFIX_PARENT_STR = ".parent_str.txt";
}

NS_FACETED_BEGIN

// as id 0 is reserved for empty value,
// members are initialized to size 1
PropValueTable::PropValueTable(
    const std::string& dirPath,
    const std::string& propName
)
    : dirPath_(dirPath)
    , propName_(propName)
    , propStrVec_(1)
    , savePropStrNum_(0)
    , parentIdVec_(1)
    , saveParentIdNum_(0)
    , childMapTable_(1)
    , valueIdTable_(1)
    , saveDocIdNum_(0)
{
}

PropValueTable::pvid_t PropValueTable::propValueId(const std::vector<izenelib::util::UString>& path)
{
    pvid_t pvId = 0;

    for (std::vector<izenelib::util::UString>::const_iterator pathIt = path.begin();
        pathIt != path.end(); ++pathIt)
    {
        PropStrMap& propStrMap = childMapTable_[pvId];
        const izenelib::util::UString& pathNode = *pathIt;

        PropStrMap::const_iterator findIt = propStrMap.find(pathNode);
        if (findIt != propStrMap.end())
        {
            pvId = findIt->second;
        }
        else
        {
            pvid_t parentId = pvId;
            pvId = propStrVec_.size();

            if (pvId != 0)
            {
                propStrMap.insert(PropStrMap::value_type(pathNode, pvId));
                propStrVec_.push_back(pathNode);
                parentIdVec_.push_back(parentId);
                childMapTable_.push_back(PropStrMap());
            }
            else
            {
                // overflow
                throw MiningException(
                    "property value count is out of range",
                    boost::lexical_cast<std::string>(propStrVec_.size()),
                    "PropValueTable::propValueId"
                );
            }
        }
    }

    return pvId;
}

bool PropValueTable::open()
{
    if (!load_container(dirPath_, propName_ + SUFFIX_PROP_STR, propStrVec_, savePropStrNum_) ||
        !load_container(dirPath_, propName_ + SUFFIX_PARENT_ID, parentIdVec_, saveParentIdNum_) ||
        !load_container(dirPath_, propName_ + SUFFIX_VALUE_ID, valueIdTable_, saveDocIdNum_))
    {
        return false;
    }

    const unsigned int valueNum = propStrVec_.size();
    if (valueNum != parentIdVec_.size())
    {
        LOG(ERROR) << "unequal property value number in propStrVec_ and parentIdVec_ ";
        return false;
    }

    childMapTable_.clear();
    childMapTable_.resize(valueNum);
    for (unsigned int i = 1; i < valueNum; ++i)
    {
        pvid_t parentId = parentIdVec_[i];
        const izenelib::util::UString& valueStr = propStrVec_[i];
        childMapTable_[parentId][valueStr] = i;
    }

    return true;
}

bool PropValueTable::flush()
{
    if (!saveParentId_(dirPath_, propName_ + SUFFIX_PARENT_STR) ||
        !save_container(dirPath_, propName_ + SUFFIX_PROP_STR, propStrVec_, savePropStrNum_) ||
        !save_container(dirPath_, propName_ + SUFFIX_PARENT_ID, parentIdVec_, saveParentIdNum_) ||
        !save_container(dirPath_, propName_ + SUFFIX_VALUE_ID, valueIdTable_, saveDocIdNum_))
    {
        return false;
    }

    return true;
}

bool PropValueTable::saveParentId_(
    const std::string& dirPath,
    const std::string& fileName
) const
{
    const unsigned int valueNum = propStrVec_.size();
    if (valueNum != parentIdVec_.size())
    {
        LOG(ERROR) << "unequal property value number in propStrVec_ and parentIdVec_ ";
        return false;
    }

    if (savePropStrNum_ >= propStrVec_.size())
        return true;

    boost::filesystem::path filePath(dirPath);
    filePath /= fileName;
    std::string pathStr = filePath.string();

    LOG(INFO) << "saving file: " << pathStr
              << ", element num: " << valueNum;

    std::ofstream ofs(pathStr.c_str());
    if (! ofs)
    {
        LOG(ERROR) << "failed opening file " << pathStr;
        return false;
    }

    std::string convertBuffer;
    for (unsigned int i = 1; i < valueNum; ++i)
    {
        propStrVec_[i].convertString(convertBuffer, UString::UTF_8);
        // columns: id, str, parentId
        ofs << i << "\t" << convertBuffer << "\t" << parentIdVec_[i] << std::endl;
    }
    return true;
}

bool PropValueTable::testDoc(docid_t docId, pvid_t labelId) const
{
    std::set<pvid_t> parentSet;
    parentIdSet(docId, parentSet);

    return parentSet.find(labelId) != parentSet.end();
}

void PropValueTable::propValuePath(pvid_t pvId, std::vector<izenelib::util::UString>& path) const
{
    // from leaf to root
    for (; pvId; pvId = parentIdVec_[pvId])
    {
        path.push_back(propStrVec_[pvId]);
    }

    // from root to leaf
    std::reverse(path.begin(), path.end());
}

NS_FACETED_END
