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

FilterManager::FilterManager(
        faceted::GroupManager* g,
        const std::string& rootpath,
        faceted::AttrManager* attr,
        NumericPropertyTableBuilder* builder)
    : groupManager_(g)
    , attrManager_(attr)
    , numericTableBuilder_(builder)
    , data_root_path_(rootpath)
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

uint32_t FilterManager::loadStrFilterInvertedData(const std::vector<std::string>& property_list, std::vector<StrFilterItemMapT>& str_filter_data)
{
    str_filter_data.resize(property_list.size());
    uint32_t max_docid = 0;
    for (size_t property_num = 0; property_num < property_list.size(); ++property_num)
    {
        const std::string& property = property_list[property_num];
        std::ifstream ifs((data_root_path_ + "/str_filter_inverted.data." + property).c_str());
        if (ifs)
        {
            LOG(INFO) << "loading filter data ";
            size_t filter_num = 0;
            ifs.read((char*)&filter_num, sizeof(filter_num));
            LOG(INFO) << "filter total num: " << filter_num;
            for (size_t i = 0; i < filter_num; ++i)
            {
                size_t key_len;
                size_t docid_len;
                ifs.read((char*)&key_len, sizeof(key_len));
                std::string key(key_len, '\0');
                ifs.read((char*)&key[0], key_len);
                ifs.read((char*)&docid_len, sizeof(docid_len));
                std::vector<uint32_t> docid_list(docid_len);
                ifs.read((char*)&docid_list[0], sizeof(docid_list[0]) * docid_len);
                //LOG(INFO) << "filter num: " << i << ", key=" << key << ", docnum: " << docid_len;
                max_docid = std::max(max_docid, *std::max_element(docid_list.begin(), docid_list.end()));
                str_filter_data[property_num].insert(std::make_pair(UString(key, UString::UTF_8), docid_list));
            }
        }
        else
        {
            LOG(INFO) << "propterty: " << property << ", no inverted file found!";
        }
    }
    return max_docid;
}

void FilterManager::saveStrFilterInvertedData(
        const std::vector<std::string>& property_list,
        const std::vector<StrFilterItemMapT>& str_filter_data) const
{
    assert(str_filter_data.size() == property_list.size());
    for (size_t property_num = 0; property_num < property_list.size(); ++property_num)
    {
        const std::string& property = property_list[property_num];
        std::ofstream ofs((data_root_path_ + "/str_filter_inverted.data." + property).c_str());
        if (ofs)
        {
            LOG(INFO) << "saveing string filter data for: " << property;
            const size_t filter_num = str_filter_data[property_num].size();
            ofs.write((const char*)&filter_num, sizeof(filter_num));
            for (StrFilterItemMapT::const_iterator cit = str_filter_data[property_num].begin();
                    cit != str_filter_data[property_num].end(); ++cit)
            {
                std::string keystr;
                cit->first.convertString(keystr, UString::UTF_8);
                const size_t key_len = keystr.size();
                const size_t docid_len = cit->second.size();
                ofs.write((const char*)&key_len, sizeof(key_len));
                ofs.write(keystr.data(), key_len);
                ofs.write((const char*)&docid_len, sizeof(docid_len));
                ofs.write((const char*)&cit->second[0], sizeof(cit->second[0]) * docid_len);
            }
        }
    }
}

FilterManager::FilterIdRange FilterManager::getStrFilterIdRange(const std::string& property, const StrFilterKeyT& strfilter_key)
{
    static const FilterIdRange empty_range;
    StrPropertyIdMapT::const_iterator cit = strtype_filterids_.find(property);
    if (cit == strtype_filterids_.end())
    {
        // no such property.
        return empty_range;
    }
    StrIdMapT::const_iterator id_cit = cit->second.find(strfilter_key);
    if ( id_cit == cit->second.end() )
    {
        // no such group
        return empty_range;
    }
    std::string outstr;
    strfilter_key.convertString(outstr, UString::UTF_8);
    LOG(INFO) << "string filter key: " << outstr << ", filter id range is: " << id_cit->second.start << "," << id_cit->second.end;
    return id_cit->second;
}

// the last_docid means where the last indexing end for. We just build the
// inverted data begin from last_docid+1.
void FilterManager::buildGroupFilterData(
        uint32_t last_docid, uint32_t max_docid,
        const std::vector<std::string>& property_list,
        std::vector<StrFilterItemMapT>& group_filter_data)
{
    if (!groupManager_) return;

    LOG(INFO) << "begin building group filter data.";
    GroupNode* group_root = new GroupNode(UString("root", UString::UTF_8));
    std::vector<GroupNode*> property_root_nodes;
    group_filter_data.resize(property_list.size());
    filter_list_.reserve(filter_list_.size() + property_list.size());
    // the relationship between group node need rebuild from docid = 1.
    for (size_t j = 0; j < property_list.size(); ++j)
    {
        group_root->appendChild(UString(property_list[j], UString::UTF_8));
        property_root_nodes.push_back(group_root->getChild(UString(property_list[j], UString::UTF_8)));

        faceted::PropValueTable* pvt = groupManager_->getPropValueTable(property_list[j]);
        if (!pvt)
        {
            LOG(INFO) << "property: " << property_list[j] << " not in group manager!";
            continue;
        }
        for (uint32_t docid = 1; docid <= max_docid; ++docid)
        {
            faceted::PropValueTable::PropIdList propids;
            // get all leaf group path
            pvt->getPropIdList(docid, propids);
            for (size_t k = 0; k < propids.size(); ++k)
            {
                std::vector<izenelib::util::UString> groupPath;
                pvt->propValuePath(propids[k], groupPath);
                if (groupPath.empty()) continue;
                GroupNode* curgroup = property_root_nodes[j];
                StrFilterKeyT groupstr = groupPath[0];
                curgroup->appendChild(groupstr);
                curgroup = curgroup->getChild(groupstr);
                assert(curgroup);
                for (size_t k = 1; k < groupPath.size(); ++k)
                {
                    groupstr.append(UString(">", UString::UTF_8)).append(groupPath[k]);
                    curgroup->appendChild(groupstr);
                    curgroup = curgroup->getChild(groupstr);
                    assert(curgroup);
                }
                if (docid <= last_docid) continue;
                StrFilterItemMapT::iterator it = group_filter_data[j].find(groupstr);
                if (it != group_filter_data[j].end())
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
        StrIdMapT& gfilterids = strtype_filterids_[property_list[j]];
        filterid_map_[property_list[j]] = filter_list_.size();
        filter_list_.resize(filter_list_.size() + 1);
        mapGroupFilterToFilterId(cur_property_group, group_filter_data[j], gfilterids);
        //printNode(property_root_nodes[j], 0, gfilterids, filter_list_);
    }
    delete group_root;
    LOG(INFO) << "finish building group filter data.";
}

void FilterManager::mapGroupFilterToFilterId(GroupNode* node, const StrFilterItemMapT& group_filter_data,
    StrIdMapT& filterids)
{
    if (node)
    {
        filterids[node->node_name_].start = filter_list_.back().size();
        StrFilterItemMapT::const_iterator cit = group_filter_data.find(node->node_name_);
        if (cit != group_filter_data.end())
            filter_list_.back().push_back(cit->second);
        for (GroupNode::constIter group_cit = node->beginChild();
                group_cit != node->endChild(); ++group_cit)
        {
            mapGroupFilterToFilterId(group_cit->second, group_filter_data, filterids);
        }
        filterids[node->node_name_].end = filter_list_.back().size();
    }
}

izenelib::util::UString FilterManager::FormatGroupPath(const std::vector<std::string>& groupPath) const
{
    izenelib::util::UString groupstr;
    if (groupPath.empty()) return groupstr;

    std::string group_tmpstr = groupPath[0];
    for (size_t k = 1; k < groupPath.size(); ++k)
    {
        if (!groupPath[k].empty())
            group_tmpstr += ">" + groupPath[k];
    }
    return UString(group_tmpstr, UString::UTF_8);
}
izenelib::util::UString FilterManager::FormatGroupPath(const std::vector<izenelib::util::UString>& groupPath) const
{
    izenelib::util::UString groupstr;
    if (groupPath.empty()) return groupstr;

    groupstr = groupPath[0];
    for (size_t k = 1; k < groupPath.size(); ++k)
    {
        if (!groupPath[k].empty())
            groupstr.append(UString(">", UString::UTF_8)).append(groupPath[k]);
    }
    return groupstr;
}

std::vector<std::vector<FilterManager::FilterDocListT> >& FilterManager::getFilterList()
{
    return filter_list_;
}

void FilterManager::clearFilterList()
{
    std::vector<std::vector<FilterDocListT> >().swap(filter_list_);
}

void FilterManager::clearFilterId()
{
    strtype_filterids_.clear();
    numtype_filterids_.clear();
    filterid_map_.clear();
}

void FilterManager::saveFilterId()
{
    for (StrPropertyIdMapT::const_iterator strtype_cit = strtype_filterids_.begin();
            strtype_cit != strtype_filterids_.end(); ++strtype_cit)
    {
        std::string savepath = data_root_path_ + "/filterid." + strtype_cit->first;
        std::ofstream ofs(savepath.c_str());
        if (ofs)
        {
            FilterType type = StrFilter;
            ofs.write((const char*)&type, sizeof(type));
            size_t num = strtype_cit->second.size();
            ofs.write((const char*)&num, sizeof(num));
            size_t id = filterid_map_[strtype_cit->first];
            ofs.write((const char*)&id, sizeof(id));
            for (StrIdMapT::const_iterator cit = strtype_cit->second.begin();
                    cit != strtype_cit->second.end(); ++cit)
            {
                std::string strkey;
                cit->first.convertString(strkey, UString::UTF_8);
                size_t key_len = strkey.size();
                ofs.write((const char*)&key_len, sizeof(key_len));
                ofs.write(strkey.data(), strkey.size());
                ofs.write((const char*)&(cit->second), sizeof(cit->second));
            }
        }
    }
    for (NumPropertyIdMapT::const_iterator numtype_cit = numtype_filterids_.begin();
            numtype_cit != numtype_filterids_.end(); ++numtype_cit)
    {
        std::string savepath = data_root_path_ + "/filterid." + numtype_cit->first;
        std::ofstream ofs(savepath.c_str());
        if (ofs)
        {
            FilterType type = NumFilter;
            ofs.write((const char*)&type, sizeof(type));
            size_t num = numtype_cit->second.size();
            ofs.write((const char*)&num, sizeof(num));
            size_t id = filterid_map_[numtype_cit->first];
            ofs.write((const char*)&id, sizeof(id));
            for (NumericIdMapT::const_iterator cit = numtype_cit->second.begin();
                    cit != numtype_cit->second.end(); ++cit)
            {
                ofs.write((const char*)&(*cit), sizeof(*cit));
            }
        }
    }
}

void FilterManager::loadFilterId(const std::vector<std::string>& property_list)
{
    for (size_t i = 0; i < property_list.size(); ++i)
    {
        const std::string& property = property_list[i];
        std::string loadpath = data_root_path_ + "/filterid." + property;
        if (!boost::filesystem::exists(loadpath))
        {
            LOG(WARNING) << "the property filter id not found. " << property;
            continue;
        }
        std::ifstream ifs(loadpath.c_str());
        if (ifs)
        {
            LOG(INFO) << "loading filter id map for property: " << property;

            FilterType type;
            ifs.read((char*)&type, sizeof(type));
            if (type < 0 || type >= LastTypeFilter)
            {
                LOG(ERROR) << "wrong filter id data. " << endl;
                continue;
            }
            size_t filter_num = 0;
            ifs.read((char*)&filter_num, sizeof(filter_num));
            LOG(INFO) << "filter id num: " << filter_num;
            size_t id = -1;
            ifs.read((char*)&id, sizeof(id));
            filterid_map_[property] = id;
            if (type == StrFilter)
            {
                StrIdMapT& strid_map = strtype_filterids_[property];
                for (size_t j = 0; j < filter_num; ++j)
                {
                    size_t key_len;
                    ifs.read((char*)&key_len, sizeof(key_len));
                    std::string key;
                    key.reserve(key_len);
                    char tmp[key_len];
                    ifs.read(tmp, key_len);
                    key.assign(tmp, key_len);
                    FilterIdRange& idrange = strid_map[UString(key, UString::UTF_8)];
                    ifs.read((char*)&idrange, sizeof(idrange));
                }
            }
            else if (type == NumFilter)
            {
                std::vector<NumFilterKeyT>& possible_keys = num_possible_keys_[property];
                possible_keys.reserve(filter_num);
                NumericIdMapT& filter_ids = numtype_filterids_[property];
                NumFilterKeyT numerickey;
                for (size_t j = 0; j < filter_num; ++j)
                {
                    ifs.read((char*)&numerickey, sizeof(numerickey));
                    FilterIdRange& idrange = filter_ids[numerickey];
                    ifs.read((char*)&idrange, sizeof(idrange));
                    filter_ids.insert(std::make_pair(numerickey, idrange));
                    possible_keys.push_back(numerickey);
                }
                std::sort(possible_keys.begin(), possible_keys.end());
            }
        }
    }
}

FilterManager::NumFilterKeyT FilterManager::formatNumericFilter(const std::string& property, double filter_num, bool tofloor) const
{
    std::map<std::string, int32_t>::const_iterator cit = num_amp_map_.find(property);
    if (cit != num_amp_map_.end())
    {
        return tofloor ? (NumFilterKeyT)std::floor(filter_num * cit->second) : (NumFilterKeyT)std::ceil(filter_num * cit->second);
    }
    else
    {
        return tofloor ? (NumFilterKeyT)std::floor(filter_num) : (NumFilterKeyT)std::ceil(filter_num);
    }
}

void FilterManager::buildDateFilterData(
        uint32_t last_docid, uint32_t max_docid,
        const std::vector< std::string >& property_list,
        std::vector<NumFilterItemMapT>& date_filter_data)
{
    if (groupManager_ == NULL)
        return;
    LOG(INFO) << "begin build date filter ... ";
    date_filter_data.resize(property_list.size());
    filter_list_.reserve(filter_list_.size() + property_list.size());
    for (size_t j = 0; j < property_list.size(); ++j)
    {
        const std::string& property = property_list[j];
        faceted::DateGroupTable* dgt = groupManager_->getDateGroupTable(property);
        if (dgt == NULL)
        {
            LOG(INFO) << "property: " << property << " is not found in date property table!";
            continue;
        }
        std::vector<NumFilterKeyT>& possible_keys = num_possible_keys_[property];
        for (uint32_t docid = last_docid + 1; docid <= max_docid; ++docid)
        {
            faceted::DateGroupTable::DateSet original_date;
            dgt->getDateSet(docid, faceted::MASK_YEAR_MONTH_DAY, original_date);
            for (faceted::DateGroupTable::DateSet::const_iterator date_it = original_date.begin();
                    date_it != original_date.end(); ++date_it)
            {
                const NumFilterKeyT& numerickey = (NumFilterKeyT)(*date_it);
                NumFilterItemMapT::iterator it = date_filter_data[j].find(numerickey);
                if (it != date_filter_data[j].end())
                {
                    it->second.push_back(docid);
                }
                else
                {
                    FilterDocListT& item = date_filter_data[j][numerickey];
                    item.push_back(docid);
                    possible_keys.push_back(numerickey);
                }
            }
        }
        // sort possible_keys in asc order
        std::sort(possible_keys.begin(), possible_keys.end(), std::less<NumFilterKeyT>());
        // map the numeric filter to filter id.
        filterid_map_[property] = filter_list_.size();
        mapNumericFilterToFilterId(date_filter_data[j], numtype_filterids_[property]);
    }
    LOG(INFO) << "finish building date filter data.";
}

void FilterManager::buildNumericFilterData(
        uint32_t last_docid, uint32_t max_docid,
        const std::vector<std::string>& property_list,
        std::vector<NumFilterItemMapT>& num_filter_data)
{
    if (numericTableBuilder_ == NULL)
    {
        LOG(INFO) << "no numericPropertyTable builder, numeric filter will not build.";
        return;
    }
    LOG(INFO) << "begin building numeric filter data.";
    num_filter_data.resize(property_list.size());
    filter_list_.reserve(filter_list_.size() + property_list.size());
    NumFilterKeyT numerickey_low;
    NumFilterKeyT numerickey_high;
    std::pair<double, double> original_key;
    NumFilterKeyT inc = 1;
    for (size_t j = 0; j < property_list.size(); ++j)
    {
        const std::string& property = property_list[j];
        LOG(INFO) << "building numeric property: " << property;
        boost::shared_ptr<NumericPropertyTableBase>& numericPropertyTable = numericTableBuilder_->createPropertyTable(property);
        if (numericPropertyTable == NULL)
        {
            LOG(INFO) << "property: " << property << " is not found in numeric property table!";
            continue;
        }
        std::vector<NumFilterKeyT>& possible_keys = num_possible_keys_[property];
        for (uint32_t docid = last_docid + 1; docid <= max_docid; ++docid)
        {
            if (!numericPropertyTable->getDoublePairValue(docid, original_key))
                continue;
            numerickey_low = formatNumericFilter(property, original_key.first);
            numerickey_high = formatNumericFilter(property, original_key.second);
            inc = 1;
            if (numerickey_high - numerickey_low > 100)
            {
                // too larger interval. just pick some.
                inc = (numerickey_high - numerickey_low) / 100;
            }
            for (NumFilterKeyT numerickey = numerickey_low; numerickey <= numerickey_high;)
            {
                FilterDocListT& item = num_filter_data[j][numerickey];
                if (item.empty())
                {
                    possible_keys.push_back(numerickey);
                }
                item.push_back(docid);
                // make sure the numerickey_high is added to index.
                if (numerickey == numerickey_high || numerickey + inc <= numerickey_high)
                    numerickey += inc;
                else
                    numerickey = numerickey_high;
            }
        }
        // sort possible_keys in asc order
        std::sort(possible_keys.begin(), possible_keys.end());
        // map the numeric filter to filter id.
        filterid_map_[property] = filter_list_.size();
        mapNumericFilterToFilterId(num_filter_data[j], numtype_filterids_[property]);
    }
    LOG(INFO) << "finish building numeric filter data.";
}

void FilterManager::mapNumericFilterToFilterId(const NumFilterItemMapT& num_filter_data, NumericIdMapT& filterids)
{
    filter_list_.resize(filter_list_.size() + 1);
    for (NumFilterItemMapT::const_iterator cit = num_filter_data.begin();
            cit != num_filter_data.end(); ++cit)
    {
        filterids[cit->first].start = filter_list_.back().size();
        if (!cit->second.empty())
            filter_list_.back().push_back(cit->second);
        filterids[cit->first].end = filter_list_.back().size();
        cout << "num: " << cit->first << ", id range: " << filterids[cit->first].start << ", "
            << filterids[cit->first].end << ", total docid size: " << cit->second.size() << endl;
    }
}

FilterManager::FilterIdRange FilterManager::getNumFilterIdRangeExactly(const std::string& property, double filter_num) const
{
    FilterIdRange empty_range;
    NumPropertyIdMapT::const_iterator cit = numtype_filterids_.find(property);
    if (cit == numtype_filterids_.end())
    {
        // no such property.
        return empty_range;
    }
    NumFilterKeyT numkey = formatNumericFilter(property, filter_num);
    NumericIdMapT::const_iterator id_cit = cit->second.find(numkey);
    if (id_cit == cit->second.end())
    {
        // no such group
        return empty_range;
    }
    LOG(INFO) << "numeric filter map to key: " << numkey << ", filter id range is: " << id_cit->second.start << "," << id_cit->second.end;
    return id_cit->second;
}

FilterManager::FilterIdRange FilterManager::getNumFilterIdRange(const std::string& property, double filter_num, bool findlarger) const
{
    FilterIdRange empty_range;
    NumPropertyIdMapT::const_iterator cit = numtype_filterids_.find(property);
    if (cit == numtype_filterids_.end())
    {
        // no such property.
        return empty_range;
    }

    std::map<std::string, std::vector<NumFilterKeyT> >::const_iterator possible_cit = num_possible_keys_.find(property);
    if (possible_cit == num_possible_keys_.end())
        return empty_range;
    NumFilterKeyT numkey = formatNumericFilter(property, filter_num, !findlarger);
    const std::vector<NumFilterKeyT>& possible_keys = possible_cit->second;
    size_t total = possible_keys.size();
    if (total == 0)
        return empty_range;
    if (findlarger)
    {
        if (numkey > possible_keys[total - 1])
            return empty_range;
    }
    else
    {
        if (numkey < possible_keys[0])
            return empty_range;
    }

    // find a nearest key
    NumFilterKeyT destkey = numkey;
    int start = 0;
    int end = (int)possible_keys.size() - 1;
    while (end >= start)
    {
        int middle = start + (end - start)/2;
        if (possible_keys[middle] > numkey)
        {
            if (findlarger)
                destkey = possible_keys[middle];
            end = middle - 1;
        }
        else if (possible_keys[middle] < numkey)
        {
            if (!findlarger)
                destkey = possible_keys[middle];
            start = middle + 1;
        }
        else
        {
            destkey = numkey;
            break;
        }
    }
    NumericIdMapT::const_iterator id_cit = cit->second.find(destkey);
    if (id_cit == cit->second.end())
    {
        // no such group
        return empty_range;
    }
    if (findlarger)
    {
        empty_range.start = id_cit->second.start;
        empty_range.end = cit->second.find(possible_keys.back())->second.end;
        LOG(INFO) << "numeric filter larger map to key: " << destkey << ", filter id range is: " << empty_range.start << "," << empty_range.end;
    }
    else
    {
        empty_range.start = cit->second.find(possible_keys[0])->second.start;
        empty_range.end = id_cit->second.end;
        LOG(INFO) << "numeric filter smaller map to key: " << destkey << ", filter id range is: " << empty_range.start << "," << empty_range.end;
    }
    return empty_range;
}

FilterManager::FilterIdRange FilterManager::getNumFilterIdRangeLarger(const std::string& property, double filter_num) const
{
    // find a nearest key that >= numkey.
    return getNumFilterIdRange(property, filter_num, true);
}

FilterManager::FilterIdRange FilterManager::getNumFilterIdRangeSmaller(const std::string& property, double filter_num) const
{
    // find a nearest key that <= numkey.
    return getNumFilterIdRange(property, filter_num, false);
}

void FilterManager::buildAttrFilterData(
        uint32_t last_docid, uint32_t max_docid,
        const std::vector<std::string>& property_list,
        std::vector<StrFilterItemMapT>& attr_filter_data)
{
    if (attrManager_ == NULL || property_list.empty())
        return;
    attr_filter_data.resize(property_list.size());
    filter_list_.reserve(filter_list_.size() + 1);
    LOG(INFO) << "begin building attribute filter data.";
    const std::string& property = property_list.front();
    const faceted::AttrTable& attr_table = attrManager_->getAttrTable();
    for (uint32_t docid = last_docid + 1; docid <= max_docid; ++docid)
    {
        faceted::AttrTable::ValueIdList attrvids;
        attr_table.getValueIdList(docid, attrvids);
        for (size_t k = 0; k < attrvids.size(); ++k)
        {
            faceted::AttrTable::nid_t attrnid = attr_table.valueId2NameId(attrvids[k]);
            StrFilterKeyT attrstr = FormatAttrPath(attr_table.nameStr(attrnid),
                    attr_table.valueStr(attrvids[k]));
            StrFilterItemMapT::iterator it = attr_filter_data[0].find(attrstr);
            if (it != attr_filter_data[0].end())
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
    filterid_map_[property] = filter_list_.size();
    mapAttrFilterToFilterId(attr_filter_data[0], filterids);
    LOG(INFO) << "finish building attribute filter data.";
}

void FilterManager::mapAttrFilterToFilterId(const StrFilterItemMapT& attr_filter_data, StrIdMapT& filterids)
{
    filter_list_.resize(filter_list_.size() + 1);
    for (StrFilterItemMapT::const_iterator cit = attr_filter_data.begin();
            cit != attr_filter_data.end(); ++cit)
    {
        filterids[cit->first].start = filter_list_.back().size();
        if (!cit->second.empty())
            filter_list_.back().push_back(cit->second);
        filterids[cit->first].end = filter_list_.back().size();
        std::string outstr;
        cit->first.convertString(outstr, UString::UTF_8);
        cout << "attribute: " << outstr << ", id range: " << filterids[cit->first].start << ", "
            << filterids[cit->first].end << ", total docid size: " << cit->second.size() << endl;
    }
}

izenelib::util::UString FilterManager::FormatAttrPath(const std::string& attrname, const std::string& attrvalue) const
{
    return UString(attrname + ": " + attrvalue, UString::UTF_8);
}

izenelib::util::UString FilterManager::FormatAttrPath(const izenelib::util::UString& attrname, const izenelib::util::UString& attrvalue) const
{
    izenelib::util::UString retstr = attrname;
    retstr.push_back(':');
    return retstr.append(attrvalue);
}

void FilterManager::setNumericAmp(const std::map<std::string, int32_t>& num_property_amp_map)
{
    num_amp_map_ = num_property_amp_map;
}

const std::map<std::string, int32_t>& FilterManager::getNumericAmp() const
{
    return num_amp_map_;
}

size_t FilterManager::getFilterId(const std::string& property) const
{
    std::map<std::string, size_t>::const_iterator it = filterid_map_.find(property);
    if (it == filterid_map_.end())
    {
        return -1;
    }
    else
    {
        return it->second;
    }
}

size_t FilterManager::filterCount() const
{
    return filterid_map_.size();
}

}
