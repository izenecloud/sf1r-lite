#include "prop_value_table.h"
#include <mining-manager/util/fcontainer.h>
#include <mining-manager/MiningException.hpp>

#include <iostream>
#include <fstream>
#include <cassert>
#include <set>

#include <boost/lexical_cast.hpp>

#include <glog/logging.h>

using namespace sf1r::faceted;

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
, valueIdTable_(1)
, saveDocIdNum_(0)
{
}

PropValueTable::pvid_t PropValueTable::propValueId(const izenelib::util::UString& value)
{
    pvid_t pvId = propStrVec_.size();

    typedef std::map<izenelib::util::UString, pvid_t> PropStrMap;
    std::pair<PropStrMap::iterator, bool> result = propStrMap_.insert(
        PropStrMap::value_type(value, pvId)
    );

    if (result.second)
    {
        if (pvId != 0)
        {
            propStrVec_.push_back(value);
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
    else
    {
        pvId = result.first->second;
    }

    return pvId;
}

bool PropValueTable::open()
{
    if (!load_container(dirPath_, propName_ + ".map", propStrVec_, savePropStrNum_)
        || !load_container(dirPath_, propName_ + ".table", valueIdTable_, saveDocIdNum_, true))
    {
        return false;
    }

    propStrMap_.clear();
    for (unsigned int i = 0; i < propStrVec_.size(); ++i)
    {
        propStrMap_[propStrVec_[i]] = i;
    }

    return true;
}

bool PropValueTable::flush()
{
    if (!save_container(dirPath_, propName_ + ".map", propStrVec_, savePropStrNum_)
        || !save_container(dirPath_, propName_ + ".table", valueIdTable_, saveDocIdNum_, true))
    {
        return false;
    }

    return true;
}

bool PropValueTable::setValueTree(const faceted::OntologyRep& valueTree)
{
    valueTree_ = valueTree;

    parentIdVec_.assign(propValueNum(), 0);
    std::list<OntologyRepItem>& itemList = valueTree_.item_list;

    // mapping from level to value id
    std::vector<pvid_t> path;
    path.push_back(0);
    std::set<pvid_t> treeIdSet;
    for (std::list<faceted::OntologyRepItem>::iterator it = itemList.begin();
        it != itemList.end(); ++it)
    {
        faceted::OntologyRepItem& item = *it;
        std::size_t currentLevel = item.level;
        BOOST_ASSERT(currentLevel >= 1 && currentLevel <= path.size());
        path.resize(currentLevel + 1);

        pvid_t pvId = propValueId(item.text);
        item.id = pvId;
        path[currentLevel] = pvId;

        if (pvId >= parentIdVec_.size())
        {
            parentIdVec_.resize(pvId+1);
        }

        if (treeIdSet.insert(pvId).second)
        {
            parentIdVec_[pvId] = path[currentLevel-1];
        }
        else
        {
            std::string utf8Str;
            item.text.convertString(utf8Str, izenelib::util::UString::UTF_8);
            LOG(ERROR) << "in <MiningSchema>::<Group>::<Property> \"" << propName_
                       << "\", duplicate <TreeNode> value \"" << utf8Str << "\"";
            return false;
        }
    }

    return true;
}

bool PropValueTable::testDoc(docid_t docId, pvid_t labelId) const
{
    std::set<pvid_t> parentSet;
    parentIdSet(docId, parentSet);

    return parentSet.find(labelId) != parentSet.end();
}

void PropValueTable::parentIdSet(docid_t docId, std::set<pvid_t>& parentSet) const
{
    if (docId >= valueIdTable_.size())
        return;

    const ValueIdList& valueIdList = valueIdTable_[docId];
    for (ValueIdList::const_iterator it = valueIdList.begin();
        it != valueIdList.end(); ++it)
    {
        pvid_t pvId = *it;
        parentSet.insert(pvId);

        if (pvId < parentIdVec_.size())
        {
            while ((pvId = parentIdVec_[pvId]))
            {
                // stop finding parent if already inserted
                if (parentSet.insert(pvId).second == false)
                    break;
            }
        }
    }
}
