#include "FilterManager.h"
#include "../group-manager/PropValueTable.h" // pvid_t
#include "../group-manager/GroupManager.h"
#include "../attr-manager/AttrManager.h"
#include <search-manager/NumericPropertyTableBuilder.h>
#include <document-manager/Document.h>
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
    , rebuild_all_(false)
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

void FilterManager::loadStrFilterInvertedData(
        const std::vector<std::string>& property_list,
        std::vector<StrFilterItemMapT>& str_filter_data,
        std::vector<uint32_t>& last_docid_list)
{
    str_filter_data.resize(property_list.size());
    last_docid_list.clear();
    last_docid_list.resize(property_list.size());

    for (size_t prop_num = 0; prop_num < property_list.size(); ++prop_num)
    {
        const std::string& property = property_list[prop_num];
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
                LOG(INFO) << "filter num: " << i << ", key=" << key << ", docnum: " << docid_len;
                last_docid_list[prop_num] = std::max(last_docid_list[prop_num], docid_list.back());
                str_filter_data[prop_num].insert(std::make_pair(UString(key, UString::UTF_8), docid_list));
            }
        }
        else
        {
            LOG(INFO) << "propterty: " << property << ", no inverted file found!";
        }
    }
}

void FilterManager::saveStrFilterInvertedData(
        const std::vector<std::string>& property_list,
        const std::vector<StrFilterItemMapT>& str_filter_data) const
{
    assert(str_filter_data.size() == property_list.size());
    for (size_t prop_num = 0; prop_num < property_list.size(); ++prop_num)
    {
        const std::string& property = property_list[prop_num];
        std::ofstream ofs((data_root_path_ + "/str_filter_inverted.data." + property).c_str());
        if (ofs)
        {
            LOG(INFO) << "saveing string filter data for: " << property;
            const size_t filter_num = str_filter_data[prop_num].size();
            ofs.write((const char*)&filter_num, sizeof(filter_num));
            for (StrFilterItemMapT::const_iterator cit = str_filter_data[prop_num].begin();
                    cit != str_filter_data[prop_num].end(); ++cit)
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

void FilterManager::setGroupFilterProperties(std::vector<std::string>& property_list)
{
    group_prop_list_ = property_list;
}

void FilterManager::setAttrFilterProperties(std::vector<std::string>& property_list)
{
    attr_prop_list_ = property_list;
}

void FilterManager::setStrFilterProperties(std::vector<std::string>& property_list)
{
    str_prop_list_ = property_list;
}

void FilterManager::setDateFilterProperties(std::vector<std::string>& property_list)
{
    date_prop_list_ = property_list;
}

void FilterManager::setNumFilterProperties(std::vector<std::string>& property_list, std::vector<int32_t>& amp_list)
{
    assert(property_list.size() == amp_list.size());
    num_prop_list_ = property_list;
    std::map<std::string, int32_t> num_amp_map;
    for (size_t i = 0; i < property_list.size(); ++i)
    {
        num_amp_map[property_list[i]] = amp_list[i];
    }
    setNumericAmp(num_amp_map);
}

void FilterManager::copyPropertyInfo(const boost::shared_ptr<FilterManager>& other)
{
    group_prop_list_ = other->group_prop_list_;
    attr_prop_list_ = other->attr_prop_list_;
    str_prop_list_ = other->str_prop_list_;
    date_prop_list_ = other->date_prop_list_;
    num_prop_list_ = other->num_prop_list_;

    prop_id_map_ = other->prop_id_map_;
    prop_list_ = other->prop_list_;
    num_amp_list_ = other->num_amp_list_;

    num_amp_map_ = other->num_amp_map_;
}

void FilterManager::generatePropertyId()
{
    generatePropertyIdForList(group_prop_list_, GROUP_ATTR_FILTER);
    generatePropertyIdForList(attr_prop_list_, GROUP_ATTR_FILTER);
    generatePropertyIdForList(str_prop_list_, STR_FILTER);
    generatePropertyIdForList(date_prop_list_, NUM_FILTER);
    generatePropertyIdForList(num_prop_list_, NUM_FILTER);

    str_filter_ids_.resize(prop_list_.size());
    num_filter_ids_.resize(prop_list_.size());
    prop_filter_str_list_.resize(prop_list_.size());
    str_key_sets_.resize(prop_list_.size());
    num_key_sets_.resize(prop_list_.size());

    filter_list_.resize(prop_list_.size());////xxxx
    str_filter_map_.resize(str_prop_list_.size());
}

void FilterManager::generatePropertyIdForList(const std::vector<std::string>& prop_list, FilterType type)
{
    for (std::vector<std::string>::const_iterator it = prop_list.begin();
            it != prop_list.end(); ++it)
    {
        size_t prop_id = prop_id_map_.size();
        if (prop_id_map_.find(*it) == prop_id_map_.end())
        {
            prop_id_map_[*it] = prop_id;
            prop_list_.resize(prop_id + 1);
            prop_list_[prop_id].first = type;
            prop_list_[prop_id].second = *it;
            if (type == NUM_FILTER)
            {
                num_amp_list_.resize(prop_id + 1, 1);
                std::map<std::string, int32_t>::const_iterator mit = num_amp_map_.find(*it);
                if (mit != num_amp_map_.end())
                {
                    num_amp_list_[prop_id] = mit->second;
                }
            }
        }
    }
}

void FilterManager::buildFilters(uint32_t last_docid, uint32_t max_docid, bool isIncre)
{
    std::vector<StrFilterItemMapT> group_filter_map;
    std::vector<StrFilterItemMapT> attr_filter_map;
    std::vector<NumFilterItemMapT> num_filter_map;
    std::vector<NumFilterItemMapT> date_filter_map;

    std::vector<uint32_t> group_last_docid_list;
    std::vector<uint32_t> attr_last_docid_list;

    if (last_docid > 0 && !isIncre)// for increase data
    {
        loadStrFilterInvertedData(group_prop_list_, group_filter_map, group_last_docid_list);
        loadStrFilterInvertedData(attr_prop_list_, attr_filter_map, attr_last_docid_list);
    }
    else
    {
        group_last_docid_list.resize(group_prop_list_.size());
        attr_last_docid_list.resize(attr_prop_list_.size());
    }

    if (isIncre)
    {
        str_filter_ids_.clear();
        str_filter_ids_.resize(prop_list_.size());

        num_filter_ids_.clear();
        num_filter_ids_.resize(prop_list_.size());

        filter_list_.clear();
        filter_list_.resize(prop_list_.size());
        
        for (std::vector<uint32_t>::iterator i = group_last_docid_list.begin(); i != group_last_docid_list.end(); ++i)
        {
            if (*i == 0)
            {
                *i = last_docid;
            }
        }
    }

    buildGroupFilters(group_last_docid_list, max_docid, group_prop_list_, group_filter_map);
    saveStrFilterInvertedData(group_prop_list_, group_filter_map);

    buildAttrFilters(attr_last_docid_list, max_docid, attr_prop_list_, attr_filter_map);
    saveStrFilterInvertedData(attr_prop_list_, attr_filter_map);
    
    if (isIncre)
    {
        buildDateFilters(last_docid, max_docid, date_prop_list_, date_filter_map);
        buildNumFilters(last_docid, max_docid, num_prop_list_, num_filter_map);
    }
    else
    {
        buildDateFilters(0, max_docid, date_prop_list_, date_filter_map);
        buildNumFilters(0, max_docid, num_prop_list_, num_filter_map);
    }
}

void FilterManager::buildStringFiltersForDoc(docid_t doc_id, const Document& doc)
{
    if (str_prop_list_.empty()) return;

    for (size_t i = 0; i < str_prop_list_.size(); ++i)
    {
        const std::string& property = str_prop_list_[i];
        Document::property_const_iterator dit = doc.findProperty(property);
        if (dit == doc.propertyEnd())
        {
            continue;
        }
        size_t prop_id = prop_id_map_[property];
        size_t n = 0, nOld = 0;
        const UString& value = dit->second.get<UString>();
        if (value.empty())
        {
            continue;
        }
        if ((n = value.find((UString::CharT)',', n)) != UString::npos)
        {
            do
            {
                if (n != nOld)
                {
                    UString sub_value = value.substr(nOld, n - nOld);
                    FilterDocListT& item = str_filter_map_[i][sub_value];
                    if (item.empty())
                    {
                        str_key_sets_[prop_id].push_back(sub_value);
                    }
                    item.push_back(doc_id);
                }
                n = value.find((UString::CharT)',', ++n);
            } while (n != UString::npos);
        }
        UString sub_value = value.substr(nOld);
        FilterDocListT& item = str_filter_map_[i][sub_value];
        if (item.empty())
        {
            str_key_sets_[prop_id].push_back(sub_value);
        }
        item.push_back(doc_id);
    }
}

void FilterManager::finishBuildStringFilters()
{
    for (size_t i = 0; i < str_prop_list_.size(); ++i)
    {
        size_t prop_id = prop_id_map_[str_prop_list_[i]];
        std::sort(str_key_sets_[prop_id].begin(), str_key_sets_[prop_id].end());
        mapAttrFilterToFilterId(str_filter_map_[i], str_filter_ids_[prop_id], filter_list_[prop_id]);
    }
    std::vector<StrFilterItemMapT>().swap(str_filter_map_);
}

// the last_docid means where the last indexing end for. We just build the
// inverted data begin from last_docid + 1.
void FilterManager::buildGroupFilters(
        const std::vector<uint32_t>& last_docid_list, uint32_t max_docid,
        const std::vector<std::string>& property_list,
        std::vector<StrFilterItemMapT>& group_filter_data)
{
    if (!groupManager_) return;

    group_filter_data.resize(property_list.size());
    
    // the relationship between group node need rebuild from docid = 1.
    GroupNode* group_root = new GroupNode(UString("root", UString::UTF_8));
    std::vector<GroupNode*> property_root_nodes;
    for (size_t j = 0; j < property_list.size(); ++j)
    {
        uint32_t last_docid_forproperty = last_docid_list[j];
        cout << "last_docid_forproperty : " << last_docid_forproperty << endl;
        const std::string& property = property_list[j];
        size_t prop_id = prop_id_map_[property];

        group_root->appendChild(UString(property, UString::UTF_8));
        property_root_nodes.push_back(group_root->getChild(UString(property, UString::UTF_8)));

        if (isUnchangedProperty(property))
        {
            LOG(INFO) << "the filter property : " << property << " do not need rebuild since no change.";
            continue;
        }

        faceted::PropValueTable* pvt = groupManager_->getPropValueTable(property);
        if (!pvt)
        {
            LOG(INFO) << "property: " << property_list[j] << " not in group manager!";
            continue;
        }
        if (groupManager_->isRebuildProp_(property))
        {
            LOG(INFO) << "clear old group data to rebuild filter data for property: " << property;
            group_filter_data[j].clear();
            last_docid_forproperty = 0;
        }
        LOG(INFO) << "building filter data, start from:" << last_docid_forproperty <<
            ", property: " << property << ", pid: " << prop_id;
        LOG(INFO) << "building filter data, end at:" << max_docid;

        // here is all build ....
        for (uint32_t docid = 1; docid <= max_docid; ++docid)
        {
            faceted::PropValueTable::PropIdList propids;
            // get all leaf group path
            pvt->getPropIdList(docid, propids);
            for (size_t k = 0; k < propids.size(); ++k)
            {
                if (docid <= last_docid_forproperty) continue;

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
                
                group_filter_data[j][groupstr].push_back(docid);
            }
        }

        // map the group filter string to filter id.
        mapGroupFilterToFilterId(
                property_root_nodes[j],
                group_filter_data[j],
                str_filter_ids_[prop_id],
                filter_list_[prop_id]);
        std::vector<StrFilterKeyT>().swap(prop_filter_str_list_[prop_id]);
        prop_filter_str_list_[prop_id].reserve(str_filter_ids_[prop_id].size() + 1);

        prop_filter_str_list_[prop_id].push_back(UString(""));
        for (StrIdMapT::const_iterator filterstr_it = str_filter_ids_[prop_id].begin();
                filterstr_it != str_filter_ids_[prop_id].end(); ++filterstr_it)
        {
            prop_filter_str_list_[prop_id].push_back(filterstr_it->first);
        }

        //printNode(property_root_nodes[j], 0, str_filter_ids_[prop_id], filter_list_[prop_id]);
    }
    delete group_root;
    LOG(INFO) << "finish building group filter data.";
}

void FilterManager::mapGroupFilterToFilterId(
        GroupNode* node,
        const StrFilterItemMapT& group_filter_data,
        StrIdMapT& filterids,
        std::vector<FilterDocListT>& filter_list)
{
    if (node)
    {
        filterids[node->node_name_].start = filter_list.size();
        string groupstr1;
        node->node_name_.convertString(groupstr1, UString::UTF_8);
        
        StrFilterItemMapT::const_iterator cit = group_filter_data.find(node->node_name_);
        if (cit != group_filter_data.end())
            filter_list.push_back(cit->second);
        for (GroupNode::constIter group_cit = node->beginChild();
                group_cit != node->endChild(); ++group_cit)
        {
            mapGroupFilterToFilterId(group_cit->second, group_filter_data, filterids, filter_list);
        }
        filterids[node->node_name_].end = filter_list.size();
    }
}

void FilterManager::buildAttrFilters(
        const std::vector<uint32_t>& last_docid_list, uint32_t max_docid,
        const std::vector<std::string>& property_list,
        std::vector<StrFilterItemMapT>& attr_filter_data)
{
    if (!attrManager_ || property_list.empty())
        return;

    uint32_t last_docid = last_docid_list[0];

    LOG(INFO) << "begin building attribute filter data.";

    attr_filter_data.resize(1);

    const std::string& property = property_list[0];
    const faceted::AttrTable& attr_table = attrManager_->getAttrTable();
    size_t prop_id = prop_id_map_[property];

    if (isUnchangedProperty(property))
    {
        LOG(INFO) << "the filter property : " << property << " do not need rebuild since no change.";
        return;
    }

    for (uint32_t docid = last_docid + 1; docid <= max_docid; ++docid)
    {
        faceted::AttrTable::ValueIdList attrvids;
        attr_table.getValueIdList(docid, attrvids);
        for (size_t k = 0; k < attrvids.size(); ++k)
        {
            StrFilterKeyT attrstr = formatAttrPath(
                    attr_table.nameStr(attr_table.valueId2NameId(attrvids[k])),
                    attr_table.valueStr(attrvids[k]));
            attr_filter_data[0][attrstr].push_back(docid);
        }
    }

    // map the attribute filter string to filter id.
    mapAttrFilterToFilterId(attr_filter_data[0], str_filter_ids_[prop_id], filter_list_[prop_id]);

    std::vector<StrFilterKeyT>().swap(prop_filter_str_list_[prop_id]);
    prop_filter_str_list_[prop_id].reserve(str_filter_ids_[prop_id].size() + 1);
    prop_filter_str_list_[prop_id].push_back(UString(""));
    for (StrIdMapT::const_iterator filterstr_it = str_filter_ids_[prop_id].begin();
            filterstr_it != str_filter_ids_[prop_id].end(); ++filterstr_it)
    {
        prop_filter_str_list_[prop_id].push_back(filterstr_it->first);
    }

    LOG(INFO) << "finish building attribute filter data.";
}

void FilterManager::mapAttrFilterToFilterId(const StrFilterItemMapT& attr_filter_data, StrIdMapT& filterids, std::vector<FilterDocListT>& filter_list)
{
    for (StrFilterItemMapT::const_iterator cit = attr_filter_data.begin();
            cit != attr_filter_data.end(); ++cit)
    {
        filterids[cit->first].start = filter_list.size();
        if (!cit->second.empty())
            filter_list.push_back(cit->second);
        filterids[cit->first].end = filter_list.size();
#ifndef NDEBUG
        std::string outstr;
        cit->first.convertString(outstr, UString::UTF_8);
        LOG(INFO) << "attribute: " << outstr << ", id range: " << filterids[cit->first].start << ", "
            << filterids[cit->first].end << ", total docid size: " << cit->second.size();
#endif
    }
}

void FilterManager::buildDateFilters( // all rebuild ....
        uint32_t last_docid, uint32_t max_docid,
        const std::vector< std::string >& property_list,
        std::vector<NumFilterItemMapT>& date_filter_data)
{
    if (!groupManager_) return;

    LOG(INFO) << "begin build date filter ... ";

    date_filter_data.resize(property_list.size());

    for (size_t j = 0; j < property_list.size(); ++j)
    {
        const std::string& property = property_list[j];
        size_t prop_id = prop_id_map_[property];

        faceted::DateGroupTable* dgt = groupManager_->getDateGroupTable(property);
        if (!dgt)
        {
            LOG(INFO) << "property: " << property << " is not found in date property table!";
            continue;
        }
        std::vector<NumFilterKeyT>& num_key_set = num_key_sets_[prop_id];
        for (uint32_t docid = last_docid + 1; docid <= max_docid; ++docid)
        {
            faceted::DateGroupTable::DateSet original_date;
            dgt->getDateSet(docid, faceted::MASK_YEAR_MONTH_DAY, original_date);
            for (faceted::DateGroupTable::DateSet::const_iterator date_it = original_date.begin();
                    date_it != original_date.end(); ++date_it)
            {
                const NumFilterKeyT& num_key = (NumFilterKeyT)(*date_it);
                FilterDocListT& item = date_filter_data[j][num_key];
                if (item.empty())
                {
                    num_key_set.push_back(num_key);
                }
                item.push_back(docid);
            }
        }
        // sort num_key_set in asc order
        std::sort(num_key_set.begin(), num_key_set.end());
        // map the numeric filter to filter id.
        mapNumericFilterToFilterId(date_filter_data[j], num_filter_ids_[prop_id], filter_list_[prop_id]);
    }
    LOG(INFO) << "finish building date filter data.";
}

void FilterManager::buildNumFilters(
        uint32_t last_docid, uint32_t max_docid,
        const std::vector<std::string>& property_list,
        std::vector<NumFilterItemMapT>& num_filter_data)
{
    if (!numericTableBuilder_)
    {
        LOG(INFO) << "no numericPropertyTable builder, numeric filter will not build.";
        return;
    }

    LOG(INFO) << "begin building numeric filter data.";

    num_filter_data.resize(property_list.size());

    NumFilterKeyT num_key_low;
    NumFilterKeyT num_key_high;
    std::pair<double, double> original_key;
    NumFilterKeyT inc = 1;
    for (size_t j = 0; j < property_list.size(); ++j)
    {
        const std::string& property = property_list[j];
        size_t prop_id = prop_id_map_[property];

        LOG(INFO) << "building numeric property: " << property;
        std::map<std::string, int32_t>::const_iterator nit = num_amp_map_.find(property);
        if (nit != num_amp_map_.end())
        {
            num_amp_list_[prop_id] = nit->second;
        }
        boost::shared_ptr<NumericPropertyTableBase>& numericPropertyTable = numericTableBuilder_->createPropertyTable(property);
        if (!numericPropertyTable)
        {
            LOG(INFO) << "property: " << property << " is not found in numeric property table!";
            continue;
        }
        std::vector<NumFilterKeyT>& num_key_set = num_key_sets_[prop_id];
        for (uint32_t docid = last_docid + 1; docid <= max_docid; ++docid)
        {
            if (!numericPropertyTable->getDoublePairValue(docid, original_key))
                continue;
            num_key_low = formatNumericFilter(prop_id, original_key.first);
            num_key_high = formatNumericFilter(prop_id, original_key.second);
            inc = 1;
            if (num_key_high - num_key_low > 100)
            {
                // too larger interval. just pick some.
                inc = (num_key_high - num_key_low) / 100;
            }
            for (NumFilterKeyT num_key = num_key_low; num_key <= num_key_high;)
            {
                FilterDocListT& item = num_filter_data[j][num_key];
                if (item.empty())
                {
                    num_key_set.push_back(num_key);
                }
                item.push_back(docid);
                // make sure the num_key_high is added to index.
                if (num_key == num_key_high || num_key + inc <= num_key_high)
                    num_key += inc;
                else
                    num_key = num_key_high;
            }
        }
        // sort num_key_set in asc order
        std::sort(num_key_set.begin(), num_key_set.end());
        // map the numeric filter to filter id.
        mapNumericFilterToFilterId(num_filter_data[j], num_filter_ids_[prop_id], filter_list_[prop_id]);
    }
    LOG(INFO) << "finish building numeric filter data.";
}

void FilterManager::mapNumericFilterToFilterId(const NumFilterItemMapT& num_filter_data, NumIdMapT& filterids, std::vector<FilterDocListT>& filter_list)
{
    for (NumFilterItemMapT::const_iterator cit = num_filter_data.begin();
            cit != num_filter_data.end(); ++cit)
    {
        cout<<"filter_list.size()---start"<<filter_list.size() << endl;
        filterids[cit->first].start = filter_list.size();
        if (!cit->second.empty())
            filter_list.push_back(cit->second);
        cout<<"filter_list.size()---end"<<filter_list.size()<<endl;
        filterids[cit->first].end = filter_list.size();
#ifndef NDEBUG
        LOG(INFO) << "num: " << cit->first << ", id range: " << filterids[cit->first].start << ", "
            << filterids[cit->first].end << ", total docid size: " << cit->second.size();
#endif
    }
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
    std::map<std::string, size_t>().swap(prop_id_map_);
    std::vector<std::pair<int32_t, std::string> >().swap(prop_list_);
    StrPropIdVecT().swap(str_filter_ids_);
    NumPropIdVecT().swap(num_filter_ids_);
    PropFilterStrVecT().swap(prop_filter_str_list_);
}

void FilterManager::saveFilterId()
{
    if (prop_list_.empty())
    {
        LOG(WARNING) << "no filter ids to be save.";
        return;
    }

    std::ofstream ofs((data_root_path_ + "/prop_list.txt").c_str());
    ofs << prop_list_.size() << std::endl;
    for (size_t i = 0; i < prop_list_.size(); ++i)
    {
        ofs << prop_list_[i].first << " " << prop_list_[i].second << std::endl;
    }

    for (size_t i = 0; i < prop_list_.size(); ++i)
    {
        std::ofstream ofs((data_root_path_ + "/filterid." + prop_list_[i].second).c_str());
        if (!ofs) continue;

        switch (prop_list_[i].first)
        {
        case GROUP_ATTR_FILTER:
            {
                size_t num = str_filter_ids_[i].size();
                ofs.write((const char*)&num, sizeof(num));
                for (StrIdMapT::const_iterator it = str_filter_ids_[i].begin();
                        it != str_filter_ids_[i].end(); ++it)
                {
                    std::string str_key;
                    it->first.convertString(str_key, UString::UTF_8);
                    saveArray_(ofs, str_key);
                    ofs.write((const char*)&(it->second), sizeof(it->second));
                }
            }
            break;

        case STR_FILTER:
            {
                size_t num = str_filter_ids_[i].size();
                ofs.write((const char*)&num, sizeof(num));
                for (StrIdMapT::const_iterator it = str_filter_ids_[i].begin();
                        it != str_filter_ids_[i].end(); ++it)
                {
                    std::string str_key;
                    it->first.convertString(str_key, UString::UTF_8);
                    saveArray_(ofs, str_key);
                    ofs.write((const char*)&(it->second), sizeof(it->second));
                }
            }
            break;

        case NUM_FILTER:
            {
                size_t num = num_filter_ids_[i].size();
                ofs.write((const char*)&num, sizeof(num));
                for (NumIdMapT::const_iterator it = num_filter_ids_[i].begin();
                        it != num_filter_ids_[i].end(); ++it)
                {
                    ofs.write((const char*)&(*it), sizeof(*it));
                }
            }
            break;

        default:
            break;
        }
    }
}

void FilterManager::loadFilterId()
{
    std::ifstream ifs((data_root_path_ + "/prop_list.txt").c_str());
    if (!ifs)
    {
        LOG(WARNING) << "no filter ids to be load.";
        return;
    }

    size_t prop_count = 0;
    ifs >> prop_count;
    std::vector<std::pair<int32_t, std::string> > tmp_prop_list(prop_count);
    for (size_t i = 0; i < prop_count; ++i)
    {
        ifs >> tmp_prop_list[i].first >> tmp_prop_list[i].second;
    }

    for (size_t i = 0; i < prop_count; ++i)
    {
        const std::string& property = tmp_prop_list[i].second;
        std::string loadpath = data_root_path_ + "/filterid." + property;
        std::ifstream ifs(loadpath.c_str());
        if (!ifs)
        {
            LOG(WARNING) << "the property filter id not found: " << property;
            rebuild_all_ = true;
            continue;
        }
        LOG(INFO) << "loading filter id map for property: " << property;
        prop_list_.push_back(tmp_prop_list[i]);
        prop_id_map_[property] = prop_list_.size() - 1;

        size_t num = 0;
        ifs.read((char*)&num, sizeof(num));
        LOG(INFO) << "filter id num: " << num;

        switch (prop_list_.back().first)
        {
        case GROUP_ATTR_FILTER:
            {
                str_filter_ids_.resize(i + 1);
                prop_filter_str_list_.resize(i + 1);
                StrIdMapT& strid_map = str_filter_ids_[i];
                // start from 1. so reserve the first.
                std::vector<StrFilterKeyT>().swap(prop_filter_str_list_[i]);
                prop_filter_str_list_[i].reserve(prop_filter_str_list_.size() + num + 1);
                prop_filter_str_list_[i].push_back(UString(""));
                for (size_t j = 0; j < num; ++j)
                {
                    std::string str_key;
                    loadArray_(ifs, str_key);
                    FilterIdRange& idrange = strid_map[UString(str_key, UString::UTF_8)];
                    ifs.read((char*)&idrange, sizeof(idrange));
                    prop_filter_str_list_[i].push_back(UString(str_key, UString::UTF_8));
                }
            }
            break;

        case STR_FILTER:
            {
                str_filter_ids_.resize(i + 1);
                str_key_sets_.resize(i + 1);
                std::vector<StrFilterKeyT>& str_key_set = str_key_sets_[i];
                str_key_set.resize(num);
                StrIdMapT& filter_ids = str_filter_ids_[i];
                for (size_t j = 0; j < num; ++j)
                {
                    std::string str_key;
                    loadArray_(ifs, str_key);
                    FilterIdRange& idrange = filter_ids[UString(str_key, UString::UTF_8)];
                    ifs.read((char*)&idrange, sizeof(idrange));
                    str_key_set[j] = str_key;
                }
            }
            break;

        case NUM_FILTER:
            {
                num_filter_ids_.resize(i + 1);
                num_key_sets_.resize(i + 1);
                num_amp_list_.resize(i + 1, 1);
                std::map<std::string, int32_t>::const_iterator mit = num_amp_map_.find(property);
                if (mit != num_amp_map_.end())
                {
                    num_amp_list_[i] = mit->second;
                }
                std::vector<NumFilterKeyT>& num_key_set = num_key_sets_[i];
                num_key_set.resize(num);
                NumIdMapT& filter_ids = num_filter_ids_[i];
                NumFilterKeyT num_key;
                for (size_t j = 0; j < num; ++j)
                {
                    ifs.read((char*)&num_key, sizeof(num_key));
                    FilterIdRange& idrange = filter_ids[num_key];
                    ifs.read((char*)&idrange, sizeof(idrange));
                    filter_ids.insert(std::make_pair(num_key, idrange));
                    num_key_set[j] = num_key;
                }
            }
            break;

        default:
            break;
        }
    }
}
void FilterManager::saveFilterList()
{
    string docid_path = data_root_path_ + "/filter.list";
    FILE* file;
    if ((file = fopen(docid_path.c_str(), "wb")) == NULL)
    {
        LOG(INFO) << "Cannot open output file"<<endl;
        return;
    }
    unsigned int size_1 = filter_list_.size();
    fwrite(&size_1, sizeof(size_1), 1, file);

    for (std::vector<std::vector<FilterDocListT> >::iterator i = filter_list_.begin(); i != filter_list_.end(); ++i)
    {
        uint32_t size_2 = i->size();
        fwrite(&size_2, sizeof(size_2), 1, file);

        for (std::vector<FilterDocListT>::iterator j = (*i).begin(); j != (*i).end(); ++j)
        {
            uint32_t size_3 = j->size();
            fwrite(&size_3, sizeof(size_3), 1, file);

            for (FilterDocListT::iterator k = (*j).begin(); k != (*j).end(); ++k)
            {
                uint32_t value = *k;
                fwrite(&value, sizeof(value), 1, file);
            }
        }
    }
}

bool FilterManager::loadFilterList()
{
    string docid_path = data_root_path_ + "/filter.list";
    FILE* file;
    if ((file = fopen(docid_path.c_str(), "rb")) == NULL)
    {
        LOG(INFO) << "Cannot open output file"<<endl;
        return false;
    }

    unsigned int size = 0;
    if (1 != fread(&size, sizeof(size), 1, file) ) return false;

    for (unsigned int i = 0; i < size; ++i)
    {
        unsigned int count = 0;
        if (1 != fread(&count, sizeof(count), 1, file) ) return false;
        std::vector<FilterDocListT> v;
        for (unsigned int j = 0; j < count; ++j)
        {
            unsigned int count_1 = 0;
            if (1 != fread(&count_1, sizeof(count_1), 1, file) ) return false;
            FilterDocListT filterDocList;
            for (unsigned int i = 0; i < count_1; ++i)
            {
                uint32_t value = 0;
                if (1 != fread(&value, sizeof(value), 1, file) ) return false;
                filterDocList.push_back(value);
            }
            v.push_back(filterDocList);
        }
        filter_list_.push_back(v);
    }
    return true;
}

izenelib::util::UString FilterManager::formatGroupPath(const std::vector<std::string>& groupPath) const
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

izenelib::util::UString FilterManager::formatGroupPath(const std::vector<izenelib::util::UString>& groupPath) const
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

izenelib::util::UString FilterManager::formatAttrPath(const std::string& attrname, const std::string& attrvalue) const
{
    return UString(attrname + ": " + attrvalue, UString::UTF_8);
}

izenelib::util::UString FilterManager::formatAttrPath(const izenelib::util::UString& attrname, const izenelib::util::UString& attrvalue) const
{
    izenelib::util::UString retstr = attrname;
    retstr.push_back(':');
    return retstr.append(attrvalue);
}

FilterManager::FilterIdRange FilterManager::getStrFilterIdRangeExact(size_t prop_id, const StrFilterKeyT& str_filter)
{
    static const FilterIdRange empty_range;
    StrIdMapT::const_iterator it = str_filter_ids_[prop_id].find(str_filter);
    std::string outstr;
    str_filter.convertString(outstr, UString::UTF_8);
    if (it == str_filter_ids_[prop_id].end())
    {
        // no such group
        LOG(INFO) << "string filter key: " << outstr << " not found in pid:" << prop_id;
        return empty_range;
    }
    LOG(INFO) << "string filter key: " << outstr << ", filter id range is: " << it->second.start << ", " << it->second.end;
    return it->second;
}

FilterManager::FilterIdRange FilterManager::getStrFilterIdRangeGreater(size_t prop_id, const StrFilterKeyT& str_filter, bool include)
{
    FilterIdRange empty_range;
    const std::vector<StrFilterKeyT>& str_key_set = str_key_sets_[prop_id];

    std::vector<StrFilterKeyT>::const_iterator it = include
        ? std::lower_bound(str_key_set.begin(), str_key_set.end(), str_filter)
        : std::upper_bound(str_key_set.begin(), str_key_set.end(), str_filter);
    if (it == str_key_set.end())
    {
        return empty_range;
    }

    StrIdMapT::const_iterator sit = str_filter_ids_[prop_id].find(*it);
    if (sit == str_filter_ids_[prop_id].end())
    {
        return empty_range;
    }

    empty_range.start = sit->second.start;
    empty_range.end = str_filter_ids_[prop_id].find(str_key_set.back())->second.end;

    std::string outstr;
    str_filter.convertString(outstr, UString::UTF_8);
    LOG(INFO) << "string filter larger map to key: " << outstr << ", filter id range is: " << empty_range.start << ", " << empty_range.end;

    return empty_range;
}

FilterManager::FilterIdRange FilterManager::getStrFilterIdRangeLess(size_t prop_id, const StrFilterKeyT& str_filter, bool include)
{
    FilterIdRange empty_range;
    const std::vector<StrFilterKeyT>& str_key_set = str_key_sets_[prop_id];

    std::vector<StrFilterKeyT>::const_iterator it = std::lower_bound(str_key_set.begin(), str_key_set.end(), str_filter);
    if (it == str_key_set.begin() && (!include || it->compare(str_filter) > 0))
    {
        return empty_range;
    }
    if (!include || it == str_key_set.end())
    {
        --it;
    }

    StrIdMapT::const_iterator sit = str_filter_ids_[prop_id].find(*it);
    if (sit == str_filter_ids_[prop_id].end())
    {
        return empty_range;
    }

    empty_range.start = str_filter_ids_[prop_id].find(str_key_set[0])->second.start;
    empty_range.end = sit->second.end;

    std::string outstr;
    str_filter.convertString(outstr, UString::UTF_8);
    LOG(INFO) << "string filter smaller map to key: " << outstr << ", filter id range is: " << empty_range.start << ", " << empty_range.end;

    return empty_range;
}

FilterManager::FilterIdRange FilterManager::getStrFilterIdRangePrefix(size_t prop_id, const StrFilterKeyT& str_filter)
{
    FilterIdRange empty_range;
    const std::vector<StrFilterKeyT>& str_key_set = str_key_sets_[prop_id];

    std::vector<StrFilterKeyT>::const_iterator it0 = std::lower_bound(str_key_set.begin(), str_key_set.end(), str_filter, PrefixCompare(str_filter.length()));
    if (it0 == str_key_set.end())
    {
        return empty_range;
    }
    std::vector<StrFilterKeyT>::const_iterator it1 = std::upper_bound(str_key_set.begin(), str_key_set.end(), str_filter, PrefixCompare(str_filter.length()));
    if (it1 == str_key_set.begin())
    {
        return empty_range;
    }
    --it1;

    StrIdMapT::const_iterator sit0 = str_filter_ids_[prop_id].find(*it0);
    if (sit0 == str_filter_ids_[prop_id].end())
    {
        return empty_range;
    }
    StrIdMapT::const_iterator sit1 = str_filter_ids_[prop_id].find(*it1);
    if (sit1 == str_filter_ids_[prop_id].end())
    {
        return empty_range;
    }

    empty_range.start = sit0->second.start;
    empty_range.end = sit1->second.end;

    std::string outstr;
    str_filter.convertString(outstr, UString::UTF_8);
    LOG(INFO) << "string filter smaller map to key: " << outstr << ", filter id range is: " << empty_range.start << ", " << empty_range.end;

    return empty_range;
}

FilterManager::NumFilterKeyT FilterManager::formatNumericFilter(size_t prop_id, double num_filter, bool tofloor) const
{
    int32_t amplifier = num_amp_list_[prop_id];
    return tofloor ? (NumFilterKeyT)std::floor(num_filter * amplifier) : (NumFilterKeyT)std::ceil(num_filter * amplifier);
}

FilterManager::FilterIdRange FilterManager::getNumFilterIdRangeExact(size_t prop_id, double num_filter) const
{
    static const FilterIdRange empty_range;
    NumFilterKeyT numkey = formatNumericFilter(prop_id, num_filter);
    NumIdMapT::const_iterator it = num_filter_ids_[prop_id].find(numkey);
    if (it == num_filter_ids_[prop_id].end())
    {
        // no such group
        return empty_range;
    }

    LOG(INFO) << "numeric filter map to key: " << numkey << ", filter id range is: " << it->second.start << "," << it->second.end;

    return it->second;
}

FilterManager::FilterIdRange FilterManager::getNumFilterIdRangeGreater(size_t prop_id, double num_filter, bool include) const
{
    FilterIdRange empty_range;
    NumFilterKeyT num_key = formatNumericFilter(prop_id, num_filter, true);
    const std::vector<NumFilterKeyT>& num_key_set = num_key_sets_[prop_id];
    if (num_key_set.size() == 0)
    {
        return empty_range;
    }

    std::vector<NumFilterKeyT>::const_iterator it = include
        ? std::lower_bound(num_key_set.begin(), num_key_set.end(), num_key)
        : std::upper_bound(num_key_set.begin(), num_key_set.end(), num_key);
    if (it == num_key_set.end())
    {
        return empty_range;
    }
    NumIdMapT::const_iterator nit = num_filter_ids_[prop_id].find(*it);
    if (nit == num_filter_ids_[prop_id].end())
    {
        return empty_range;
    }

    empty_range.start = nit->second.start;
    empty_range.end = num_filter_ids_[prop_id].find(num_key_set.back())->second.end;

    LOG(INFO) << "numeric filter larger map to key: " << *it << ", filter id range is: " << empty_range.start << "," << empty_range.end;

    return empty_range;
}

FilterManager::FilterIdRange FilterManager::getNumFilterIdRangeLess(size_t prop_id, double num_filter, bool include) const
{
    FilterIdRange empty_range;
    NumFilterKeyT num_key = formatNumericFilter(prop_id, num_filter, false);
    const std::vector<NumFilterKeyT>& num_key_set = num_key_sets_[prop_id];

    if (num_key_set.size() == 0)
    {
        return empty_range;
    }

    std::vector<NumFilterKeyT>::const_iterator it = std::lower_bound(num_key_set.begin(), num_key_set.end(), num_key);
    if (it == num_key_set.begin() && (!include || *it > num_filter))
    {
        return empty_range;
    }
    if (it == num_key_set.end())
    {
        --it;
    }
    NumIdMapT::const_iterator nit = num_filter_ids_[prop_id].find(*it);
    if (nit == num_filter_ids_[prop_id].end())
    {
        return empty_range;
    }
    
    empty_range.start = num_filter_ids_[prop_id].find(num_key_set[0])->second.start;

    if (include && *it == num_key)
    {
        empty_range.end = nit->second.end;
    }
    else
    {
        empty_range.end = nit->second.end - 1;
    }

    LOG(INFO) << "numeric filter smaller map to key: " << *it << ", filter id range is: " << empty_range.start << "," << empty_range.end;

    return empty_range;
}

void FilterManager::setNumericAmp(const std::map<std::string, int32_t>& num_amp_map)
{
    num_amp_map_ = num_amp_map;
    num_amp_list_.resize(num_amp_map_.size(), 1);
    std::map<std::string, int32_t>::const_iterator mit = num_amp_map_.begin();
    while (mit != num_amp_map_.end())
    {
        size_t prop_id = getPropertyId(mit->first);
        if (prop_id != (size_t)-1)
        {
            num_amp_list_[prop_id] = mit->second;
        }
        ++mit;
    }
}

const std::map<std::string, int32_t>& FilterManager::getNumericAmp() const
{
    return num_amp_map_;
}

size_t FilterManager::getPropertyId(const std::string& property) const
{
    std::map<std::string, size_t>::const_iterator it = prop_id_map_.find(property);
    if (it == prop_id_map_.end())
    {
        return -1;
    }
    else
    {
        return it->second;
    }
}

size_t FilterManager::getAttrPropertyId() const
{
    if (attr_prop_list_.empty())
    {
        return -1;
    }
    else
    {
        return getPropertyId(attr_prop_list_[0]);
    }
}

size_t FilterManager::propertyCount() const
{
    return prop_id_map_.size();
}

izenelib::util::UString FilterManager::getPropFilterString(size_t prop_id, size_t filter_strid) const
{
    izenelib::util::UString result;
    if (prop_id >= prop_filter_str_list_.size())
        return result;
    // filter_strid start from 1. just like the document id.
    assert(filter_strid >= 1);
    if (filter_strid >= prop_filter_str_list_[prop_id].size())
        return result;
    return prop_filter_str_list_[prop_id][filter_strid];
}

size_t FilterManager::getMaxPropFilterStrId(size_t prop_id) const
{
    if (prop_id >= prop_filter_str_list_.size())
        return 0;
    return prop_filter_str_list_[prop_id].size() - 1;
}

void FilterManager::addUnchangedProperty(const std::string& property)
{
    if (rebuild_all_ || (groupManager_ && groupManager_->isRebuildProp_(property)))
    {
        // rebuild property always changed.
        return;
    }
    unchanged_prop_list_.insert(property);
}

void FilterManager::clearUnchangedProperties()
{
    unchanged_prop_list_.clear();
}

bool FilterManager::isUnchangedProperty(const std::string& property) const
{
    return unchanged_prop_list_.find(property) != unchanged_prop_list_.end();
}

void FilterManager::swapUnchangedFilter(FilterManager* old_filter)
{
    for (std::set<std::string>::const_iterator cit = unchanged_prop_list_.begin();
            cit != unchanged_prop_list_.end(); ++cit)
    {
        size_t prop_id = getPropertyId(*cit);
        if (prop_id == (size_t)-1)
        {
            LOG(ERROR) << "swap filter string id failed for non-exist property: " << *cit;
            continue;
        }
        str_filter_ids_[prop_id].swap(old_filter->str_filter_ids_[prop_id]);
        prop_filter_str_list_[prop_id].swap(old_filter->prop_filter_str_list_[prop_id]);
        LOG(INFO) << "property filter data swapped: " << *cit;
    }
}

void FilterManager::setRebuildFlag(const FilterManager* old_filter)
{
    rebuild_all_ = !old_filter || old_filter->rebuild_all_;
}

void FilterManager::clearRebuildFlag()
{
    rebuild_all_ = false;
}

}
