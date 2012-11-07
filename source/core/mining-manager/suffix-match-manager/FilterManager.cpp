#include "FilterManager.h"
#include "../group-manager/PropValueTable.h" // pvid_t
#include "../group-manager/GroupManager.h"
#include "../attr-manager/AttrManager.h"
#include <boost/filesystem.hpp>
#include <fstream>
#include <iostream>

using namespace izenelib::util;

namespace sf1r
{

FilterManager::FilterManager(faceted::GroupManager* g, const std::string& rootpath)
    :groupManager_(g),
    attrManager_(NULL),
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

void FilterManager::setAttrManager(faceted::AttrManager* attr)
{
    attrManager_ = attr;
}

faceted::AttrManager* FilterManager::getAttrManager()
{
    return attrManager_;
}

uint32_t FilterManager::loadGroupFilterInvertedData(const std::vector<std::string>& propertys, std::vector<StrFilterItemMapT>& group_filter_data)
{
    group_filter_data.resize(propertys.size());
    uint32_t max_docid = 0;
    for(size_t property_num = 0; property_num < propertys.size(); ++property_num)
    {
        const std::string& property = propertys[property_num];
        std::ifstream ifs((data_root_path_ + "/group_filter_inverted.data." + property).c_str());
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
                group_filter_data[property_num].insert(std::make_pair(UString(key, UString::UTF_8), docid_list));
            }
        }
        else
        {
            LOG(INFO) << "propterty:" << property << ", no inverted file found!";
        }
    }
    return max_docid;
}

void FilterManager::saveGroupFilterInvertedData(const std::vector<std::string>& propertys, 
    const std::vector<StrFilterItemMapT>& group_filter_data) const
{
    assert(group_filter_data.size() == propertys.size());
    for(size_t property_num = 0; property_num < propertys.size(); ++property_num)
    {
        const std::string& property = propertys[property_num];
        std::ofstream ofs((data_root_path_ + "/group_filter_inverted.data." + property).c_str());
        if (ofs)
        {
            LOG(INFO) << "saveing group filter data for: " << property;
            const size_t filter_num = group_filter_data[property_num].size();
            ofs.write((const char*)&filter_num, sizeof(filter_num));
            StrFilterItemMapT::const_iterator cit = group_filter_data[property_num].begin();
            while(cit != group_filter_data[property_num].end())
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

// the last_docid means where the last indexing end for. We just build the 
// inverted data begin from last_docid+1.
void FilterManager::buildGroupFilterData(uint32_t last_docid, uint32_t max_docid,
    std::vector< std::string >& propertys,
    std::vector<StrFilterItemMapT>& group_filter_data)
{
    if(groupManager_ == NULL)
        return;
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
                for(size_t j = 0; j < filter_num; ++j)
                {
                    float numberkey;
                    ifs.read((char*)&numberkey, sizeof(numberkey));
                    FilterIdRange idrange;
                    ifs.read((char*)&idrange.start, sizeof(uint32_t));
                    ifs.read((char*)&idrange.end, sizeof(uint32_t));
                    numbertype_filterids_[property].insert(std::make_pair(numberkey, idrange));
                }
            }
        }
    }
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

FilterManager::FilterIdRange FilterManager::getGroupFilterIdRange(const std::string& property, const izenelib::util::UString& grouppath)
{
    FilterIdRange empty_range;
    StrPropertyIdMapT::const_iterator cit = strtype_filterids_.find(property);
    if(cit == strtype_filterids_.end())
    {
        // no such property.
        return empty_range;
    }
    StrIdMapT::const_iterator id_cit = cit->second.find(grouppath);
    if( id_cit == cit->second.end() )
    {
        // no such group
        return empty_range;
    }
    std::string outstr;
    grouppath.convertString(outstr, UString::UTF_8);
    LOG(INFO) << "group path : " << outstr << ", filter id range is:" << id_cit->second.start << "," << id_cit->second.end;
    return id_cit->second;
}

FilterManager::FilterIdRange FilterManager::getPriceFilterIdRange()
{
    return FilterIdRange();
}

std::vector<FilterManager::FilterDocListT>& FilterManager::getAllFilterInvertedData()
{
    return all_inverted_filter_data_;
}

void FilterManager::clearAllFilterInvertedData()
{
    all_inverted_filter_data_.clear();
}

}


