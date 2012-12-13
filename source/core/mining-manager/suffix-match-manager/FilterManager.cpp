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
    , need_rebuild_all_filter_(false)
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

std::vector<uint32_t> FilterManager::loadStrFilterInvertedData(const std::vector<std::string>& property_list, std::vector<StrFilterItemMapT>& str_filter_data)
{
    str_filter_data.resize(property_list.size());
    std::vector<uint32_t> max_docid_list(property_list.size(), 0);
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
                max_docid_list[property_num] = std::max(max_docid_list[property_num], *std::max_element(docid_list.begin(), docid_list.end()));
                str_filter_data[property_num].insert(std::make_pair(UString(key, UString::UTF_8), docid_list));
            }
        }
        else
        {
            LOG(INFO) << "propterty: " << property << ", no inverted file found!";
        }
    }
    return max_docid_list;
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

// the last_docid means where the last indexing end for. We just build the
// inverted data begin from last_docid+1.
void FilterManager::buildGroupFilterData(
        const std::vector<uint32_t>& last_docid_list, uint32_t max_docid,
        const std::vector<std::string>& property_list,
        std::vector<StrFilterItemMapT>& group_filter_data)
{
    if (!groupManager_) return;

    LOG(INFO) << "begin building group filter data.";

    group_filter_data.resize(property_list.size());
    prop_list_.resize(prop_id_map_.size() + property_list.size());
    strtype_filterids_.resize(prop_list_.size());
    filter_list_.resize(prop_list_.size());
    prop_filterstr_text_list_.resize(prop_list_.size());
    // the relationship between group node need rebuild from docid = 1.
    GroupNode* group_root = new GroupNode(UString("root", UString::UTF_8));
    std::vector<GroupNode*> property_root_nodes;
    for (size_t j = 0; j < property_list.size(); ++j)
    {
        uint32_t last_docid_forproperty = last_docid_list[j];
        const std::string& property = property_list[j];
        size_t prop_id = prop_id_map_.size();
        prop_id_map_[property] = prop_id;
        prop_list_[prop_id].first = STR_FILTER;
        prop_list_[prop_id].second = property;

        group_root->appendChild(UString(property, UString::UTF_8));
        property_root_nodes.push_back(group_root->getChild(UString(property, UString::UTF_8)));

        if(isUnchangedProperty(property))
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
        LOG(INFO) << "building filter data, start from:" << last_docid_forproperty << ", property: " << property;

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
                if (docid <= last_docid_forproperty) continue;
                group_filter_data[j][groupstr].push_back(docid);
            }
        }
        
        // map the group filter string to filter id.
        mapGroupFilterToFilterId(
                property_root_nodes[j],
                group_filter_data[j],
                strtype_filterids_[prop_id],
                filter_list_[prop_id]);

        std::vector<StrFilterKeyT>().swap(prop_filterstr_text_list_[prop_id]);
        prop_filterstr_text_list_[prop_id].reserve(strtype_filterids_[prop_id].size() + 1);

        prop_filterstr_text_list_[prop_id].push_back(UString(""));
        for(StrIdMapT::const_iterator filterstr_it = strtype_filterids_[prop_id].begin();
            filterstr_it != strtype_filterids_[prop_id].end(); ++ filterstr_it)
        {
            prop_filterstr_text_list_[prop_id].push_back(filterstr_it->first);
        }

        printNode(property_root_nodes[j], 0, strtype_filterids_[prop_id], filter_list_[prop_id]);
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

void FilterManager::buildAttrFilterData(
        const std::vector<uint32_t>& last_docid_list, uint32_t max_docid,
        const std::vector<std::string>& property_list,
        std::vector<StrFilterItemMapT>& attr_filter_data)
{
    if (!attrManager_ || property_list.empty())
        return;

    uint32_t last_docid = last_docid_list.front();

    LOG(INFO) << "begin building attribute filter data.";

    attr_filter_data.resize(1);
    prop_list_.resize(prop_id_map_.size() + 1);
    strtype_filterids_.resize(prop_list_.size());
    filter_list_.resize(prop_list_.size());
    prop_filterstr_text_list_.resize(prop_list_.size());

    const std::string& property = property_list.front();
    const faceted::AttrTable& attr_table = attrManager_->getAttrTable();
    size_t prop_id = prop_id_map_.size();
    prop_id_map_[property] = prop_id;
    prop_list_[prop_id].first = STR_FILTER;
    prop_list_[prop_id].second = property;

    if(isUnchangedProperty(property))
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
            StrFilterKeyT attrstr = FormatAttrPath(
                    attr_table.nameStr(attr_table.valueId2NameId(attrvids[k])),
                    attr_table.valueStr(attrvids[k]));
            attr_filter_data[0][attrstr].push_back(docid);
        }
    }
    // map the attribute filter string to filter id.
    mapAttrFilterToFilterId(attr_filter_data[0], strtype_filterids_[prop_id], filter_list_[prop_id]);

    std::vector<StrFilterKeyT>().swap(prop_filterstr_text_list_[prop_id]);
    prop_filterstr_text_list_[prop_id].reserve(strtype_filterids_[prop_id].size() + 1);
    prop_filterstr_text_list_[prop_id].push_back(UString(""));
    for(StrIdMapT::const_iterator filterstr_it = strtype_filterids_[prop_id].begin();
        filterstr_it != strtype_filterids_[prop_id].end(); ++ filterstr_it)
    {
        prop_filterstr_text_list_[prop_id].push_back(filterstr_it->first);
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
        std::string outstr;
        cit->first.convertString(outstr, UString::UTF_8);
        cout << "attribute: " << outstr << ", id range: " << filterids[cit->first].start << ", "
            << filterids[cit->first].end << ", total docid size: " << cit->second.size() << endl;
    }
}

void FilterManager::buildDateFilterData(
        uint32_t last_docid, uint32_t max_docid,
        const std::vector< std::string >& property_list,
        std::vector<NumFilterItemMapT>& date_filter_data)
{
    if (!groupManager_) return;

    LOG(INFO) << "begin build date filter ... ";

    date_filter_data.resize(property_list.size());
    prop_list_.resize(prop_id_map_.size() + property_list.size());
    num_amp_list_.resize(prop_list_.size(), 1);
    numtype_filterids_.resize(prop_list_.size());
    num_possible_keys_.resize(prop_list_.size());
    filter_list_.resize(prop_list_.size());

    for (size_t j = 0; j < property_list.size(); ++j)
    {
        const std::string& property = property_list[j];
        size_t prop_id = prop_id_map_.size();
        prop_id_map_[property] = prop_id;
        prop_list_[prop_id].first = NUM_FILTER;
        prop_list_[prop_id].second = property;
        faceted::DateGroupTable* dgt = groupManager_->getDateGroupTable(property);
        if (!dgt)
        {
            LOG(INFO) << "property: " << property << " is not found in date property table!";
            continue;
        }
        std::vector<NumFilterKeyT>& possible_keys = num_possible_keys_[prop_id];
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
                    date_filter_data[j][numerickey].push_back(docid);
                    possible_keys.push_back(numerickey);
                }
            }
        }
        // sort possible_keys in asc order
        std::sort(possible_keys.begin(), possible_keys.end());
        // map the numeric filter to filter id.
        mapNumericFilterToFilterId(date_filter_data[j], numtype_filterids_[prop_id], filter_list_[prop_id]);
    }
    LOG(INFO) << "finish building date filter data.";
}

void FilterManager::buildNumericFilterData(
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
    prop_list_.resize(prop_id_map_.size() + property_list.size());
    num_amp_list_.resize(prop_list_.size(), 1);
    numtype_filterids_.resize(prop_list_.size());
    num_possible_keys_.resize(prop_list_.size());
    filter_list_.resize(prop_list_.size());

    NumFilterKeyT numerickey_low;
    NumFilterKeyT numerickey_high;
    std::pair<double, double> original_key;
    NumFilterKeyT inc = 1;
    for (size_t j = 0; j < property_list.size(); ++j)
    {
        const std::string& property = property_list[j];
        size_t prop_id = prop_id_map_.size();
        prop_id_map_[property] = prop_id;
        prop_list_[prop_id].first = NUM_FILTER;
        prop_list_[prop_id].second = property;
        LOG(INFO) << "building numeric property: " << property;
        std::map<std::string, int32_t>::const_iterator it = num_amp_map_.find(property);
        if (it != num_amp_map_.end())
        {
            num_amp_list_[prop_id] = it->second;
        }
        boost::shared_ptr<NumericPropertyTableBase>& numericPropertyTable = numericTableBuilder_->createPropertyTable(property);
        if (!numericPropertyTable)
        {
            LOG(INFO) << "property: " << property << " is not found in numeric property table!";
            continue;
        }
        std::vector<NumFilterKeyT>& possible_keys = num_possible_keys_[prop_id];
        for (uint32_t docid = last_docid + 1; docid <= max_docid; ++docid)
        {
            if (!numericPropertyTable->getDoublePairValue(docid, original_key))
                continue;
            numerickey_low = formatNumericFilter(prop_id, original_key.first);
            numerickey_high = formatNumericFilter(prop_id, original_key.second);
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
        mapNumericFilterToFilterId(num_filter_data[j], numtype_filterids_[prop_id], filter_list_[prop_id]);
    }
    LOG(INFO) << "finish building numeric filter data.";
}

void FilterManager::mapNumericFilterToFilterId(const NumFilterItemMapT& num_filter_data, NumIdMapT& filterids, std::vector<FilterDocListT>& filter_list)
{
    for (NumFilterItemMapT::const_iterator cit = num_filter_data.begin();
            cit != num_filter_data.end(); ++cit)
    {
        filterids[cit->first].start = filter_list.size();
        if (!cit->second.empty())
            filter_list.push_back(cit->second);
        filterids[cit->first].end = filter_list.size();
        cout << "num: " << cit->first << ", id range: " << filterids[cit->first].start << ", "
            << filterids[cit->first].end << ", total docid size: " << cit->second.size() << endl;
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
    std::vector<std::pair<int, std::string> >().swap(prop_list_);
    StrPropIdVecT().swap(strtype_filterids_);
    NumPropIdVecT().swap(numtype_filterids_);
    PropFilterStrVecT().swap(prop_filterstr_text_list_);
}

void FilterManager::saveFilterId()
{
    for (size_t i = 0; i < prop_list_.size(); ++i)
    {
        std::ofstream ofs((data_root_path_ + "/filterid." + prop_list_[i].second).c_str());
        if (!ofs) continue;

        ofs.write((const char*)&i, sizeof(i));
        ofs.write((const char*)&prop_list_[i].first, sizeof(prop_list_[i].first));

        switch (prop_list_[i].first)
        {
        case STR_FILTER:
            {
                size_t num = strtype_filterids_[i].size();
                ofs.write((const char*)&num, sizeof(num));
                for (StrIdMapT::const_iterator it = strtype_filterids_[i].begin();
                        it != strtype_filterids_[i].end(); ++it)
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
                size_t num = numtype_filterids_[i].size();
                ofs.write((const char*)&num, sizeof(num));
                for (NumIdMapT::const_iterator it = numtype_filterids_[i].begin();
                        it != numtype_filterids_[i].end(); ++it)
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

void FilterManager::loadFilterId(const std::vector<std::string>& property_list)
{
    for (size_t i = 0; i < property_list.size(); ++i)
    {
        const std::string& property = property_list[i];
        std::string loadpath = data_root_path_ + "/filterid." + property;
        std::ifstream ifs(loadpath.c_str());
        if (!ifs)
        {
            LOG(WARNING) << "the property filter id not found: " << property;
            need_rebuild_all_filter_ = true;
            continue;
        }
        LOG(INFO) << "loading filter id map for property: " << property;

        size_t prop_id = -1;
        ifs.read((char*)&prop_id, sizeof(prop_id));
        if (prop_id == (size_t)-1)
        {
            LOG(ERROR) << "wrong filter id data. " << endl;
            need_rebuild_all_filter_ = true;
            continue;
        }
        if (prop_id >= prop_list_.size())
        {
            prop_list_.resize(prop_id + 1);
        }
        int& type = prop_list_[prop_id].first;
        type = FILTER_TYPE_COUNT;
        ifs.read((char*)&type, sizeof(type));
        if (type < 0 || type >= FILTER_TYPE_COUNT)
        {
            LOG(ERROR) << "wrong filter id data. " << endl;
            need_rebuild_all_filter_ = true;
            continue;
        }
        prop_list_[prop_id].second = property;
        prop_id_map_[property] = prop_id;
        size_t num = 0;
        ifs.read((char*)&num, sizeof(num));
        LOG(INFO) << "filter id num: " << num;
        switch (type)
        {
        case STR_FILTER:
            {
                if (prop_id >= strtype_filterids_.size())
                {
                    strtype_filterids_.resize(prop_id + 1);
                    prop_filterstr_text_list_.resize(prop_id + 1);
                }
                StrIdMapT& strid_map = strtype_filterids_[prop_id];
                // start from 1. so reserve the first.
                std::vector<StrFilterKeyT>().swap(prop_filterstr_text_list_[prop_id]);
                prop_filterstr_text_list_[prop_id].reserve(num + 1);
                prop_filterstr_text_list_[prop_id].push_back(UString(""));
                for (size_t j = 0; j < num; ++j)
                {
                    std::string str_key;
                    loadArray_(ifs, str_key);
                    FilterIdRange& idrange = strid_map[UString(str_key, UString::UTF_8)];
                    ifs.read((char*)&idrange, sizeof(idrange));
                    prop_filterstr_text_list_[prop_id].push_back(UString(str_key, UString::UTF_8));
                }
            }
            break;

        case NUM_FILTER:
            {
                if (prop_id >= numtype_filterids_.size())
                {
                    numtype_filterids_.resize(prop_id + 1);
                    num_possible_keys_.resize(prop_id + 1);
                    num_amp_list_.resize(prop_id + 1, 1);
                }
                std::map<std::string, int32_t>::const_iterator mit = num_amp_map_.find(property);
                if (mit != num_amp_map_.end())
                {
                    num_amp_list_[prop_id] = mit->second;
                }
                std::vector<NumFilterKeyT>& possible_keys = num_possible_keys_[prop_id];
                possible_keys.resize(num);
                NumIdMapT& filter_ids = numtype_filterids_[prop_id];
                NumFilterKeyT numerickey;
                for (size_t j = 0; j < num; ++j)
                {
                    ifs.read((char*)&numerickey, sizeof(numerickey));
                    FilterIdRange& idrange = filter_ids[numerickey];
                    ifs.read((char*)&idrange, sizeof(idrange));
                    filter_ids.insert(std::make_pair(numerickey, idrange));
                    possible_keys[j] = numerickey;
                }
                std::sort(possible_keys.begin(), possible_keys.end());
            }
            break;

        default:
            break;
        }
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

FilterManager::FilterIdRange FilterManager::getStrFilterIdRange(size_t prop_id, const StrFilterKeyT& strfilter_key)
{
    static const FilterIdRange empty_range;
    StrIdMapT::const_iterator it = strtype_filterids_[prop_id].find(strfilter_key);
    if (it == strtype_filterids_[prop_id].end())
    {
        // no such group
        return empty_range;
    }
    std::string outstr;
    strfilter_key.convertString(outstr, UString::UTF_8);
    LOG(INFO) << "string filter key: " << outstr << ", filter id range is: " << it->second.start << ", " << it->second.end;
    return it->second;
}

FilterManager::NumFilterKeyT FilterManager::formatNumericFilter(size_t prop_id, double filter_num, bool tofloor) const
{
    int32_t amplifier = num_amp_list_[prop_id];
    return tofloor ? (NumFilterKeyT)std::floor(filter_num * amplifier) : (NumFilterKeyT)std::ceil(filter_num * amplifier);
}

FilterManager::FilterIdRange FilterManager::getNumFilterIdRangeExactly(size_t prop_id, double filter_num) const
{
    static const FilterIdRange empty_range;
    NumFilterKeyT numkey = formatNumericFilter(prop_id, filter_num);
    NumIdMapT::const_iterator it = numtype_filterids_[prop_id].find(numkey);
    if (it == numtype_filterids_[prop_id].end())
    {
        // no such group
        return empty_range;
    }
    LOG(INFO) << "numeric filter map to key: " << numkey << ", filter id range is: " << it->second.start << "," << it->second.end;
    return it->second;
}

FilterManager::FilterIdRange FilterManager::getNumFilterIdRangeLarger(size_t prop_id, double filter_num) const
{
    // find a nearest key that >= numkey.
    return getNumFilterIdRange(prop_id, filter_num, true);
}

FilterManager::FilterIdRange FilterManager::getNumFilterIdRangeSmaller(size_t prop_id, double filter_num) const
{
    // find a nearest key that <= numkey.
    return getNumFilterIdRange(prop_id, filter_num, false);
}

FilterManager::FilterIdRange FilterManager::getNumFilterIdRange(size_t prop_id, double filter_num, bool findlarger) const
{
    FilterIdRange empty_range;
    NumFilterKeyT numkey = formatNumericFilter(prop_id, filter_num, !findlarger);
    const std::vector<NumFilterKeyT>& possible_keys = num_possible_keys_[prop_id];
    size_t total = possible_keys.size();
    if (total == 0) return empty_range;
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
    NumIdMapT::const_iterator it = numtype_filterids_[prop_id].find(destkey);
    if (it == numtype_filterids_[prop_id].end())
    {
        // no such group
        return empty_range;
    }
    if (findlarger)
    {
        empty_range.start = it->second.start;
        empty_range.end = numtype_filterids_[prop_id].find(possible_keys.back())->second.end;
        LOG(INFO) << "numeric filter larger map to key: " << destkey << ", filter id range is: " << empty_range.start << "," << empty_range.end;
    }
    else
    {
        empty_range.start = numtype_filterids_[prop_id].find(possible_keys[0])->second.start;
        empty_range.end = it->second.end;
        LOG(INFO) << "numeric filter smaller map to key: " << destkey << ", filter id range is: " << empty_range.start << "," << empty_range.end;
    }
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
        if(prop_id != (size_t)-1)
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

size_t FilterManager::propertyCount() const
{
    return prop_id_map_.size();
}

izenelib::util::UString FilterManager::getPropFilterString(size_t prop_id, size_t filter_strid) const
{
    izenelib::util::UString result;
    if(prop_id >= prop_filterstr_text_list_.size())
        return result;
    // filter_strid start from 1. just like the document id.
    assert(filter_strid >= 1);
    if(filter_strid >= prop_filterstr_text_list_[prop_id].size())
        return result;
    return prop_filterstr_text_list_[prop_id][filter_strid];
}

size_t FilterManager::getMaxPropFilterStrId(size_t prop_id) const
{
    if(prop_id >= prop_filterstr_text_list_.size())
        return 0;
    return prop_filterstr_text_list_[prop_id].size() - 1;
}

void FilterManager::addUnchangedProperty(const std::string& property)
{
    if(need_rebuild_all_filter_)
        return;
    if(groupManager_ && groupManager_->isRebuildProp_(property))
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
    std::set<std::string>::const_iterator cit = unchanged_prop_list_.begin();
    for(; cit != unchanged_prop_list_.end(); ++cit)
    {
        size_t prop_id = getPropertyId(*cit);
        if(prop_id == (size_t)-1)
        {
            LOG(ERROR) << "swap filter string id failed for non-exist property: " << *cit;
            continue;
        }
        strtype_filterids_[prop_id].swap(old_filter->strtype_filterids_[prop_id]);
        prop_filterstr_text_list_[prop_id].swap(old_filter->prop_filterstr_text_list_[prop_id]);
        LOG(INFO) << "property filter data swapped: " << *cit;
    }
}

void FilterManager::setRebuildFlag(const FilterManager* old_filter)
{
    if(old_filter == NULL)
    {
        need_rebuild_all_filter_ = true;
        return;
    }
    need_rebuild_all_filter_ = old_filter->need_rebuild_all_filter_;
}

void FilterManager::clearRebuildFlag()
{
    need_rebuild_all_filter_ = false;
}

}
