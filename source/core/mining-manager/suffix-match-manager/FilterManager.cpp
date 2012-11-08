#include "FilterManager.h"
#include "../group-manager/PropValueTable.h" // pvid_t
#include "../group-manager/GroupManager.h"
#include "../attr-manager/AttrManager.h"
#include <search-manager/NumericPropertyTableBuilder.h>
#include <boost/filesystem.hpp>
#include <fstream>
#include <iostream>

using namespace izenelib::util;

namespace sf1r
{

FilterManager::FilterManager(faceted::GroupManager* g, const std::string& rootpath,
    faceted::AttrManager* attr, NumericPropertyTableBuilder* builder)
    :groupManager_(g),
    attrManager_(attr),
    numericTableBuilder_(builder),
    data_root_path_(rootpath)
{
}

FilterManager::~FilterManager()
{
}

faceted::GroupManager* FilterManager::getGroupManager() const 
{
    return groupManager_;
}

faceted::AttrManager* FilterManager::getAttrManager() const
{
    return attrManager_;
}

NumericPropertyTableBuilder* FilterManager::getNumericTableBuilder() const
{
    return numericTableBuilder_;
}

uint32_t FilterManager::loadStrFilterInvertedData(const std::vector<std::string>& propertys, std::vector<StrFilterItemMapT>& str_filter_data)
{
    str_filter_data.resize(propertys.size());
    uint32_t max_docid = 0;
    for(size_t property_num = 0; property_num < propertys.size(); ++property_num)
    {
        const std::string& property = propertys[property_num];
        std::ifstream ifs((data_root_path_ + "/str_filter_inverted.data." + property).c_str());
        if (ifs)
        {
            LOG(INFO) << "loading filter data ";
            size_t filter_num = 0;
            ifs.read((char*)&filter_num, sizeof(filter_num));
            LOG(INFO) << "filter total num : " << filter_num;
            for(size_t i = 0; i < filter_num; ++i)
            {
                size_t key_len;
                size_t docid_len;
                ifs.read((char*)&key_len, sizeof(key_len));
                std::string key;
                key.reserve(key_len);
                char tmp[key_len];
                ifs.read(tmp, key_len);
                key.assign(tmp, key_len);
                ifs.read((char*)&docid_len, sizeof(docid_len));
                std::vector<uint32_t> docid_list;
                docid_list.reserve(docid_len);
                //LOG(INFO) << "filter num : " << i << ", key=" << key << ", docnum:" << docid_len;
                for(size_t j = 0; j < docid_len; ++j)
                {
                    uint32_t did;
                    ifs.read((char*)&did, sizeof(did));
                    docid_list.push_back(did);
                    max_docid = did > max_docid ? did:max_docid;
                }
                str_filter_data[property_num].insert(std::make_pair(UString(key, UString::UTF_8), docid_list));
            }
        }
        else
        {
            LOG(INFO) << "propterty:" << property << ", no inverted file found!";
        }
    }
    return max_docid;
}

void FilterManager::saveStrFilterInvertedData(const std::vector<std::string>& propertys, 
    const std::vector<StrFilterItemMapT>& str_filter_data) const
{
    assert(str_filter_data.size() == propertys.size());
    for(size_t property_num = 0; property_num < propertys.size(); ++property_num)
    {
        const std::string& property = propertys[property_num];
        std::ofstream ofs((data_root_path_ + "/str_filter_inverted.data." + property).c_str());
        if (ofs)
        {
            LOG(INFO) << "saveing string filter data for: " << property;
            const size_t filter_num = str_filter_data[property_num].size();
            ofs.write((const char*)&filter_num, sizeof(filter_num));
            StrFilterItemMapT::const_iterator cit = str_filter_data[property_num].begin();
            while(cit != str_filter_data[property_num].end())
            {
                const StrFilterItemT& item = *cit;
                std::string keystr;
                item.first.convertString(keystr, UString::UTF_8);
                const size_t key_len = keystr.size();
                const size_t docid_len = item.second.size();
                ofs.write((const char*)&key_len, sizeof(key_len));
                ofs.write(keystr.data(), key_len);
                ofs.write((const char*)&docid_len, sizeof(docid_len));
                //LOG(INFO) << "filter key=" << keystr << ", docnum:" << docid_len;
                for(size_t j = 0; j < docid_len; ++j)
                {
                    const uint32_t& did = item.second[j];
                    ofs.write((const char*)&did, sizeof(did));
                }
                ++cit;
            }
        }
    }
}

FilterManager::FilterIdRange FilterManager::getStrFilterIdRange(const std::string& property, const StrFilterKeyT& strfilter_key)
{
    FilterIdRange empty_range;
    StrPropertyIdMapT::const_iterator cit = strtype_filterids_.find(property);
    if(cit == strtype_filterids_.end())
    {
        // no such property.
        return empty_range;
    }
    StrIdMapT::const_iterator id_cit = cit->second.find(strfilter_key);
    if( id_cit == cit->second.end() )
    {
        // no such group
        return empty_range;
    }
    std::string outstr;
    strfilter_key.convertString(outstr, UString::UTF_8);
    LOG(INFO) << "string filter key : " << outstr << ", filter id range is:" << id_cit->second.start << "," << id_cit->second.end;
    return id_cit->second;
}

// the last_docid means where the last indexing end for. We just build the 
// inverted data begin from last_docid+1.
void FilterManager::buildGroupFilterData(uint32_t last_docid, uint32_t max_docid,
    const std::vector< std::string >& propertys,
    std::vector<StrFilterItemMapT>& group_filter_data)
{
    if(groupManager_ == NULL)
        return;
    LOG(INFO) << "begin building group filter data.";
    GroupNode* group_root = new GroupNode(UString("root", UString::UTF_8));
    std::vector<GroupNode*> property_root_nodes;
    group_filter_data.resize(propertys.size());
    // the relationship between group node need rebuild from docid = 1.
    for(size_t j = 0; j < propertys.size(); ++j)
    {
        group_root->appendChild(UString(propertys[j], UString::UTF_8));
        property_root_nodes.push_back(group_root->getChild(UString(propertys[j], UString::UTF_8)));

        faceted::PropValueTable* pvt = groupManager_->getPropValueTable(propertys[j]);
        if(pvt == NULL)
        {
            LOG(INFO) << "property : " << propertys[j] << " not in group manager!";
            continue;
        }
        for(uint32_t docid = 1; docid <= max_docid; ++docid)
        {
            faceted::PropValueTable::PropIdList propids;
            // get all leaf group path
            pvt->getPropIdList(docid, propids);
            for(size_t k = 0; k < propids.size(); ++k)
            {
                std::vector< izenelib::util::UString > groupPath;
                pvt->propValuePath(propids[k], groupPath);
                if(groupPath.empty())
                    continue;
                GroupNode* curgroup = property_root_nodes[j];
                StrFilterKeyT groupstr = groupPath[0];
                curgroup->appendChild(groupstr);
                curgroup = curgroup->getChild(groupstr);
                assert(curgroup);
                for(size_t k = 1; k < groupPath.size(); ++k)
                {
                    groupstr.append(UString(">", UString::UTF_8)).append(groupPath[k]);
                    curgroup->appendChild(groupstr);
                    curgroup = curgroup->getChild(groupstr);
                    assert(curgroup);
                }
                //if(propids.size() > 1)
                //{
                //    std::string outstr;
                //    groupstr.convertString(outstr, UString::UTF_8);
                //    LOG(INFO) << "the docid belong two different leaf group ";
                //    LOG(INFO) << docid << ", " << outstr;
                //}
                if(docid <= last_docid)
                    continue;
                StrFilterItemMapT::iterator it = group_filter_data[j].find(groupstr);
                if(it != group_filter_data[j].end())
                {
                    it->second.push_back(docid);
                }
                else
                {
                    // new group 
                    FilterDocListT& item = group_filter_data[j][groupstr];
                    item.push_back(docid);
                }
            }
        }
        // map the group filter string to filter id.
        GroupNode* cur_property_group = property_root_nodes[j];
        StrIdMapT& gfilterids = strtype_filterids_[propertys[j]];
        mapGroupFilterToFilterId(cur_property_group, group_filter_data[j], gfilterids);        
        printNode(property_root_nodes[j], 0, gfilterids, all_inverted_filter_data_);
    }
    delete group_root;
    LOG(INFO) << "finish building group filter data.";
}

void FilterManager::mapGroupFilterToFilterId(GroupNode* node, const StrFilterItemMapT& group_filter_data,
    StrIdMapT& filterids)
{
    if(node)
    {
        filterids[node->node_name_].start = all_inverted_filter_data_.size();
        StrFilterItemMapT::const_iterator cit = group_filter_data.find(node->node_name_);
        if(cit != group_filter_data.end())
            all_inverted_filter_data_.push_back(cit->second);
        GroupNode::constIter group_cit = node->beginChild();
        while(group_cit != node->endChild())
        {
            mapGroupFilterToFilterId(group_cit->second, group_filter_data, filterids);
            ++group_cit;
        }
        filterids[node->node_name_].end = all_inverted_filter_data_.size();
    }
}

izenelib::util::UString FilterManager::FormatGroupPath(std::vector<std::string>& groupPath) const
{
    izenelib::util::UString groupstr;
    if(groupPath.empty())
        return groupstr;
    std::string group_tmpstr = groupPath[0];
    for(size_t k = 1; k < groupPath.size(); ++k)
    {
        group_tmpstr += ">" + groupPath[k];
    }
    return UString(group_tmpstr, UString::UTF_8);
}
izenelib::util::UString FilterManager::FormatGroupPath(std::vector<izenelib::util::UString>& groupPath) const
{
    izenelib::util::UString groupstr;
    if(groupPath.empty())
        return groupstr;
    groupstr = groupPath[0];
    for(size_t k = 1; k < groupPath.size(); ++k)
    {
        groupstr.append(UString(">", UString::UTF_8)).append(groupPath[k]);
    }
    return groupstr;
}

std::vector<FilterManager::FilterDocListT>& FilterManager::getAllFilterInvertedData()
{
    return all_inverted_filter_data_;
}

void FilterManager::clearAllFilterInvertedData()
{
    all_inverted_filter_data_.clear();
}

void FilterManager::clearFilterId()
{
    strtype_filterids_.clear();
    numbertype_filterids_.clear();
}

void FilterManager::saveFilterId()
{
    StrPropertyIdMapT::const_iterator strtype_cit = strtype_filterids_.begin();
    while( strtype_cit != strtype_filterids_.end() )
    {
        std::string savepath = data_root_path_ + "/filterid." + strtype_cit->first;
        std::ofstream ofs(savepath.c_str());
        if(ofs)
        {
            FilterType type = StrFilter;
            ofs.write((const char*)&type, sizeof(type));
            size_t num = strtype_cit->second.size();
            ofs.write((const char*)&num, sizeof(size_t));
            StrIdMapT::const_iterator cit = strtype_cit->second.begin();
            while(cit != strtype_cit->second.end())
            {
                std::string strkey;
                cit->first.convertString(strkey, UString::UTF_8);
                size_t key_len = strkey.size();
                ofs.write((const char*)&key_len, sizeof(size_t));
                ofs.write(strkey.data(), strkey.size());
                ofs.write((const char*)&(cit->second.start), sizeof(uint32_t));
                ofs.write((const char*)&(cit->second.end), sizeof(uint32_t));
                ++cit;
            }
        }
        ++strtype_cit;
    }
    NumPropertyIdMapT::const_iterator numtype_cit = numbertype_filterids_.begin();
    while( numtype_cit != numbertype_filterids_.end() )
    {
        std::string savepath = data_root_path_ + "/filterid." + numtype_cit->first;
        std::ofstream ofs(savepath.c_str());
        if(ofs)
        {
            FilterType type = NumFilter;
            ofs.write((const char*)&type, sizeof(type));
            size_t num = numtype_cit->second.size();
            ofs.write((const char*)&num, sizeof(size_t));
            NumberIdMapT::const_iterator cit = numtype_cit->second.begin();
            while(cit != numtype_cit->second.end())
            {
                ofs.write((const char*)&cit->first, sizeof(cit->first));
                ofs.write((const char*)&(cit->second.start), sizeof(uint32_t));
                ofs.write((const char*)&(cit->second.end), sizeof(uint32_t));
                ++cit;
            }
        }
        ++numtype_cit;
    }
}

void FilterManager::loadFilterId(const std::vector<std::string> propertys)
{
    for(size_t i = 0; i < propertys.size(); ++i)
    {
        const std::string& property = propertys[i];
        std::string loadpath = data_root_path_ + "/filterid." + property;
        if (!boost::filesystem::exists(loadpath))
        {
            LOG(WARNING) << "the property filter id not found. " << property;
            continue;
        }
        std::ifstream ifs(loadpath.c_str());
        if(ifs)
        {
            LOG(INFO) << "loading filter id map for property: " << property;

            FilterType type;
            ifs.read((char*)&type, sizeof(type));
            if(type < 0 || type >= LastTypeFilter)
            {
                LOG(ERROR) << "wrong filter id data. " << endl;
                continue;
            }
            size_t filter_num = 0;
            ifs.read((char*)&filter_num, sizeof(filter_num));
            LOG(INFO) << "filter id num : " << filter_num;
            if(type == StrFilter)
            {
                for(size_t j = 0; j < filter_num; ++j)
                {
                    size_t key_len;
                    ifs.read((char*)&key_len, sizeof(key_len));
                    std::string key;
                    key.reserve(key_len);
                    char tmp[key_len];
                    ifs.read(tmp, key_len);
                    key.assign(tmp, key_len);
                    FilterIdRange idrange;
                    ifs.read((char*)&idrange.start, sizeof(uint32_t));
                    ifs.read((char*)&idrange.end, sizeof(uint32_t));
                    strtype_filterids_[property].insert(std::make_pair(UString(key, UString::UTF_8), idrange));
                }
            }
            else if(type == NumFilter)
            {
                std::vector<NumFilterKeyT>& possible_keys = num_possible_keys_[property];
                possible_keys.reserve(filter_num);
                for(size_t j = 0; j < filter_num; ++j)
                {
                    NumFilterKeyT numberkey;
                    ifs.read((char*)&numberkey, sizeof(numberkey));
                    FilterIdRange idrange;
                    ifs.read((char*)&idrange.start, sizeof(uint32_t));
                    ifs.read((char*)&idrange.end, sizeof(uint32_t));
                    numbertype_filterids_[property].insert(std::make_pair(numberkey, idrange));
                    possible_keys.push_back(numberkey);
                }
                std::sort(possible_keys.begin(), possible_keys.end(), std::less<NumFilterKeyT>());
            }
        }
    }
}


FilterManager::NumFilterKeyT FilterManager::formatNumberFilter(const std::string& property,
    float filter_num, bool tofloor) const
{
    // need add config for each property to get the amplification
    if(property == "Score")
    {
        return tofloor ? (NumFilterKeyT)std::floor(filter_num * 100) : (NumFilterKeyT)std::ceil(filter_num * 100);
    }
    else if(property == "Price")
    {
        return tofloor?(NumFilterKeyT)std::floor(filter_num):(NumFilterKeyT)std::ceil(filter_num);
    }
    else
    {
        return tofloor ? (NumFilterKeyT)std::floor(filter_num):(NumFilterKeyT)std::ceil(filter_num);
    }
}

void FilterManager::buildNumberFilterData(uint32_t last_docid, uint32_t max_docid, const std::vector< std::string >& propertys,
    std::vector<NumFilterItemMapT>& num_filter_data)
{
    if(numericTableBuilder_ == NULL)
    {
        LOG(INFO) << "no numericPropertyTable builder, number filter will not build.";
        return;
    }
    LOG(INFO) << "begin building number filter data.";
    num_filter_data.resize(propertys.size());
    for(size_t j = 0; j < propertys.size(); ++j)
    {
        const std::string& property = propertys[j];
        boost::shared_ptr<NumericPropertyTableBase>& numericPropertyTable = numericTableBuilder_->createPropertyTable(property);
        if (numericPropertyTable == NULL)
        {
            LOG(INFO) << "property : " << property << " is not found in number property table!";
            continue;
        }
        std::vector<NumFilterKeyT>& possible_keys = num_possible_keys_[property];
        for(uint32_t docid = last_docid + 1; docid <= max_docid; ++docid)
        {
            NumFilterKeyT numberkey_low = 0;
            NumFilterKeyT numberkey_high = 0;
            std::pair<float, float> original_key;
            if(!numericPropertyTable->getFloatPairValue(docid, original_key))
                continue;
            numberkey_low = formatNumberFilter(property, original_key.first);
            numberkey_high = formatNumberFilter(property, original_key.second);
            NumFilterKeyT inc = 1;
            if(numberkey_high - numberkey_low > 100)
            {
                // too larger interval. just pick some.
                inc = (numberkey_high - numberkey_low)/100;
            }
            for(NumFilterKeyT numberkey = numberkey_low; numberkey <= numberkey_high;)
            {
                NumFilterItemMapT::iterator it = num_filter_data[j].find(numberkey);
                if(it != num_filter_data[j].end())
                {
                    it->second.push_back(docid);
                }
                else
                {
                    FilterDocListT& item = num_filter_data[j][numberkey];
                    item.push_back(docid);
                    possible_keys.push_back(numberkey);
                }
                // make sure the numberkey_high is added to index.
                if(numberkey == numberkey_high || numberkey + inc <= numberkey_high)
                    numberkey += inc;
                else
                    numberkey = numberkey_high;
            }
        }
        // sort possible_keys in asc order
        std::sort(possible_keys.begin(), possible_keys.end(), std::less<NumFilterKeyT>());
        // map the number filter to filter id.
        NumberIdMapT& num_filterids = numbertype_filterids_[property];
        mapNumberFilterToFilterId(num_filter_data[j], num_filterids);        
    }
    LOG(INFO) << "finish building number filter data.";

}

void FilterManager::mapNumberFilterToFilterId(const NumFilterItemMapT& num_filter_data, NumberIdMapT& filterids)
{
    NumFilterItemMapT::const_iterator cit = num_filter_data.begin();
    while(cit != num_filter_data.end())
    {
        filterids[cit->first].start = all_inverted_filter_data_.size();
        if(!cit->second.empty())
            all_inverted_filter_data_.push_back(cit->second);
        filterids[cit->first].end = all_inverted_filter_data_.size();
        cout << "num :" << cit->first << ", id range:" << filterids[cit->first].start <<
            ", " << filterids[cit->first].end << ", total docid size:" << cit->second.size() << endl;

        ++cit;
    }
}

FilterManager::FilterIdRange FilterManager::getNumFilterIdRangeExactly(const std::string& property, float filter_num) const
{
    FilterIdRange empty_range;
    NumPropertyIdMapT::const_iterator cit = numbertype_filterids_.find(property);
    if(cit == numbertype_filterids_.end())
    {
        // no such property.
        return empty_range;
    }
    NumFilterKeyT numkey = formatNumberFilter(property, filter_num);
    NumberIdMapT::const_iterator id_cit = cit->second.find(numkey);
    if( id_cit == cit->second.end() )
    {
        // no such group
        return empty_range;
    }
    LOG(INFO) << "number filter map to key: " << numkey << ", filter id range is:" << id_cit->second.start << "," << id_cit->second.end;
    return id_cit->second;
}

FilterManager::FilterIdRange FilterManager::getNumFilterIdRange(const std::string& property, float filter_num, bool findlarger) const
{
    FilterIdRange empty_range;
    NumPropertyIdMapT::const_iterator cit = numbertype_filterids_.find(property);
    if(cit == numbertype_filterids_.end())
    {
        // no such property.
        return empty_range;
    }
    
    std::map<std::string, std::vector<NumFilterKeyT> >::const_iterator possible_cit = num_possible_keys_.find(property);
    if(possible_cit == num_possible_keys_.end())
        return empty_range;
    NumFilterKeyT numkey = formatNumberFilter(property, filter_num, !findlarger);
    const std::vector<NumFilterKeyT>& possible_keys = possible_cit->second;
    size_t total = possible_keys.size();
    if(total == 0)
        return empty_range;
    if(findlarger)
    {
        if(numkey > possible_keys[total - 1])
            return empty_range;
    }
    else
    {
        if(numkey < possible_keys[0])
            return empty_range;
    }

    // find a nearest key 
    NumFilterKeyT destkey = numkey;
    int start = 0;
    int end = (int)possible_keys.size() - 1;
    while(end >= start)
    {
        int middle = start + (end - start)/2;
        if(possible_keys[middle] > numkey)
        {
            if(findlarger)
                destkey = possible_keys[middle];
            end = middle - 1;
        }
        else if(possible_keys[middle] < numkey)
        {
            if(!findlarger)
                destkey = possible_keys[middle];
            start = middle + 1;
        }
        else
        {
            destkey = numkey;
            break;
        }
    }
    NumberIdMapT::const_iterator id_cit = cit->second.find(destkey);
    if( id_cit == cit->second.end() )
    {
        // no such group
        return empty_range;
    }
    if(findlarger)
    {
        empty_range.start = id_cit->second.start;
        empty_range.end = cit->second.find(possible_keys.back())->second.end;
        LOG(INFO) << "number filter larger map to key: " << destkey << ", filter id range is:" << empty_range.start << "," << empty_range.end;
    }
    else
    {
        empty_range.start = cit->second.find(possible_keys[0])->second.start;
        empty_range.end = id_cit->second.end;
        LOG(INFO) << "number filter smaller map to key: " << destkey << ", filter id range is:" << empty_range.start << "," << empty_range.end;
    }
    return empty_range;
}

FilterManager::FilterIdRange FilterManager::getNumFilterIdRangeLarger(const std::string& property, float filter_num) const
{
    // find a nearest key that >= numkey.
    return getNumFilterIdRange(property, filter_num, true);
}

FilterManager::FilterIdRange FilterManager::getNumFilterIdRangeSmaller(const std::string& property, float filter_num) const
{
    // find a nearest key that <= numkey.
    return getNumFilterIdRange(property, filter_num, false);
}

void FilterManager::buildAttrFilterData(uint32_t last_docid, uint32_t max_docid, const std::vector<std::string>& propertys,
    std::vector<StrFilterItemMapT>& attr_filter_data)
{
    if(attrManager_ == NULL || propertys.empty())
        return;
    attr_filter_data.resize(propertys.size());
    LOG(INFO) << "begin building attribute filter data.";
    const std::string property = propertys.back();
    const faceted::AttrTable& attr_table = attrManager_->getAttrTable();
    for(uint32_t docid = last_docid + 1; docid <= max_docid; ++docid)
    {
        faceted::AttrTable::ValueIdList attrvids;
        attr_table.getValueIdList(docid, attrvids);
        for(size_t k = 0; k < attrvids.size(); ++k)
        {
            faceted::AttrTable::nid_t attrnid = attr_table.valueId2NameId(attrvids[k]);
            StrFilterKeyT attrstr = FormatAttrPath(attr_table.nameStr(attrnid),
                attr_table.valueStr(attrvids[k]));
            //if(attrvids.size() > 1)
            //{
            //    std::string outstr;
            //    attrstr.convertString(outstr, UString::UTF_8);
            //    LOG(INFO) << "the docid has two different attribute";
            //    LOG(INFO) << docid << ", " << outstr;
            //}
            StrFilterItemMapT::iterator it = attr_filter_data[0].find(attrstr);
            if(it != attr_filter_data[0].end())
            {
                it->second.push_back(docid);
            }
            else
            {
                // new attribute
                FilterDocListT& item = attr_filter_data[0][attrstr];
                item.push_back(docid);
            }
        }
    }
    // map the attribute filter string to filter id.
    StrIdMapT& filterids = strtype_filterids_[property];
    mapAttrFilterToFilterId(attr_filter_data[0], filterids);        
    LOG(INFO) << "finish building attribute filter data.";
}

void FilterManager::mapAttrFilterToFilterId(const StrFilterItemMapT& attr_filter_data, StrIdMapT& filterids)
{
    StrFilterItemMapT::const_iterator cit = attr_filter_data.begin();
    while(cit != attr_filter_data.end())
    {
        filterids[cit->first].start = all_inverted_filter_data_.size();
        if(!cit->second.empty())
            all_inverted_filter_data_.push_back(cit->second);
        filterids[cit->first].end = all_inverted_filter_data_.size();
        std::string outstr;
        cit->first.convertString(outstr, UString::UTF_8);
        cout << "attribute :" << outstr << ", id range:" << filterids[cit->first].start <<
            ", " << filterids[cit->first].end << ", total docid size:" << cit->second.size() << endl;

        ++cit;
    }
}

izenelib::util::UString FilterManager::FormatAttrPath(const std::string& attrname,
    const std::string& attrvalue) const
{
    return UString(attrname + ":" + attrvalue, UString::UTF_8);
}

izenelib::util::UString FilterManager::FormatAttrPath(const izenelib::util::UString& attrname,
    const izenelib::util::UString& attrvalue) const
{
    izenelib::util::UString retstr = attrname;
    return retstr.append(UString(":", UString::UTF_8)).append(attrvalue);
}

}


