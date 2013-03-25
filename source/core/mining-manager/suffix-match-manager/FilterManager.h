#ifndef SF1R_MINING_SUFFIX_FILTERMANAGER_H_
#define SF1R_MINING_SUFFIX_FILTERMANAGER_H_

#include <common/type_defs.h>
#include <boost/shared_ptr.hpp>

namespace sf1r
{
namespace faceted
{
class GroupManager;
class AttrManager;
}
class NumericPropertyTableBuilder;
class Document;

class GroupNode
{
public:
    typedef std::map<izenelib::util::UString, GroupNode*> ChildContainerT;
    typedef ChildContainerT::iterator Iter;
    typedef ChildContainerT::const_iterator constIter;

    ChildContainerT child_nodes_;
    // the name is full path from root to the current node
    izenelib::util::UString  node_name_;

    GroupNode(const izenelib::util::UString& name)
        : node_name_(name)
    {
    }
    ~GroupNode()
    {
        for (ChildContainerT::iterator it = child_nodes_.begin();
                it != child_nodes_.end(); ++it)
        {
            delete it->second;
        }
        child_nodes_.clear();
    }
    Iter beginChild()
    {
        return child_nodes_.begin();
    }
    Iter endChild()
    {
        return child_nodes_.end();
    }
    constIter beginChild() const
    {
        return child_nodes_.begin();
    }
    constIter endChild() const
    {
        return child_nodes_.end();
    }

    bool appendChild(GroupNode* child)
    {
        if (!child || child_nodes_.find(child->node_name_) != child_nodes_.end())
            return false;
        child_nodes_[child->node_name_] = child;
        return true;
    }
    bool appendChild(const izenelib::util::UString& node_name)
    {
        if (child_nodes_.find(node_name) != child_nodes_.end())
            return false;
        child_nodes_[node_name] = new GroupNode(node_name);
        return true;
    }
    bool removeChild(const izenelib::util::UString& node_name)
    {
        ChildContainerT::iterator it = child_nodes_.find(node_name);
        if (it == child_nodes_.end())
            return false;
        delete it->second;
        child_nodes_.erase(it);
    }
    GroupNode* getChild(const izenelib::util::UString& node_name)
    {
        ChildContainerT::iterator it = child_nodes_.find(node_name);
        if (it == child_nodes_.end())
            return NULL;
        return it->second;
    }
};

class FilterManager
{
public:
    enum FilterType
    {
        GROUP_ATTR_FILTER = 0,
        STR_FILTER,
        NUM_FILTER,

        FILTER_TYPE_COUNT
    };
    struct FilterIdRange
    {
        uint32_t start;
        uint32_t end;
        FilterIdRange()
            : start(0), end(0)
        {
        }
    };

    typedef std::vector<uint32_t> FilterDocListT;
    typedef izenelib::util::UString StrFilterKeyT;
    typedef int32_t NumFilterKeyT;
    typedef std::pair<StrFilterKeyT, FilterDocListT> StrFilterItemT;
    typedef std::pair<NumFilterKeyT, FilterDocListT> NumFilterItemT;
    typedef std::map<StrFilterKeyT, FilterDocListT> StrFilterItemMapT;
    typedef std::map<NumFilterKeyT, FilterDocListT> NumFilterItemMapT;

    FilterManager(
            faceted::GroupManager* g,
            const std::string& rootpath,
            faceted::AttrManager* attr,
            NumericPropertyTableBuilder* numericTableBuilder);
    ~FilterManager();

    void loadStrFilterInvertedData(const std::vector<std::string>& property, std::vector<StrFilterItemMapT>& str_filter_data, std::vector<uint32_t>& last_docid_list);
    void saveStrFilterInvertedData(const std::vector<std::string>& property, const std::vector<StrFilterItemMapT>& str_filte_data) const;

    void setGroupFilterProperties(std::vector<std::string>& property_list);
    void setAttrFilterProperties(std::vector<std::string>& property_list);
    void setStrFilterProperties(std::vector<std::string>& property_list);
    void setDateFilterProperties(std::vector<std::string>& property_list);
    void setNumFilterProperties(std::vector<std::string>& property_list, std::vector<int32_t>& amp_list);

    void copyPropertyInfo(const boost::shared_ptr<FilterManager>& other);
    void generatePropertyId();

    void buildFilters(uint32_t last_docid, uint32_t max_docid);

    void buildStringFiltersForDoc(docid_t doc_id, const Document& doc);
    void finishBuildStringFilters();

    void buildGroupFilters(
            const std::vector<uint32_t>& last_docid_list, uint32_t max_docid,
            const std::vector<std::string>& property_list,
            std::vector<StrFilterItemMapT>& group_filter_data);
    void buildAttrFilters(
            const std::vector<uint32_t>& last_docid_list, uint32_t max_docid,
            const std::vector<std::string>& property_list,
            std::vector<StrFilterItemMapT>& attr_filter_data);
    void buildDateFilters(
            uint32_t last_docid, uint32_t max_docid,
            const std::vector<std::string>& property_list,
            std::vector<NumFilterItemMapT>& date_filter_data);
    void buildNumFilters(
            uint32_t last_docid, uint32_t max_docid,
            const std::vector<std::string>& property_list,
            std::vector<NumFilterItemMapT>& num_filter_data);


    izenelib::util::UString formatGroupPath(const std::vector<izenelib::util::UString>& groupPath) const;
    izenelib::util::UString formatGroupPath(const std::vector<std::string>& groupPath) const;

    izenelib::util::UString formatAttrPath(
            const izenelib::util::UString& attrname,
            const izenelib::util::UString& attrvalue) const;
    izenelib::util::UString formatAttrPath(
            const std::string& attrname,
            const std::string& attrvalue) const;

    FilterIdRange getStrFilterIdRangeExact(size_t prop_id, const izenelib::util::UString& str_filter);
    FilterIdRange getStrFilterIdRangeGreater(size_t prop_id, const izenelib::util::UString& str_filter, bool include);
    FilterIdRange getStrFilterIdRangeLess(size_t prop_id, const izenelib::util::UString& str_filter, bool include);
    FilterIdRange getStrFilterIdRangePrefix(size_t prop_id, const izenelib::util::UString& str_filter);

    FilterIdRange getNumFilterIdRangeExact(size_t prop_id, double filter_num) const;
    FilterIdRange getNumFilterIdRangeGreater(size_t prop_id, double filter_num, bool include) const;
    FilterIdRange getNumFilterIdRangeLess(size_t prop_id, double filter_num, bool include) const;

    std::vector<std::vector<FilterDocListT> >& getFilterList();
    const std::vector<std::pair<int32_t, std::string> >& getProp_list()
    {
        return prop_list_;
    }
    void clearFilterList();

    void clearFilterId();
    void saveFilterId();
    void loadFilterId();

    faceted::GroupManager* getGroupManager() const;
    faceted::AttrManager* getAttrManager() const;
    NumericPropertyTableBuilder* getNumericTableBuilder() const;

    void setNumericAmp(const std::map<std::string, int32_t>& num_amp_list);
    const std::map<std::string, int32_t>& getNumericAmp() const;

    izenelib::util::UString getPropFilterString(size_t prop_id, size_t filter_strid) const;
    size_t getMaxPropFilterStrId(size_t prop_id) const;

    size_t getPropertyId(const std::string& property) const;
    size_t getAttrPropertyId() const;
    size_t propertyCount() const;
    void addUnchangedProperty(const std::string& property);
    void clearUnchangedProperties();
    bool isUnchangedProperty(const std::string& property) const;
    void swapUnchangedFilter(FilterManager* old_filter);
    void setRebuildFlag(const FilterManager* old_filter = NULL);
    void clearRebuildFlag();
    inline const std::set<std::string>& getUnchangedProperties() const
    {
        return unchanged_prop_list_;
    }

    inline bool isGroupProp(const std::string& prop) const
    {
        return std::binary_search(group_prop_list_.begin(), group_prop_list_.end(), prop);
    }
    inline bool isStringProp(const std::string& prop) const
    {
        return std::binary_search(str_prop_list_.begin(), str_prop_list_.end(), prop);
    }
    inline bool isNumericProp(const std::string& prop) const
    {
        return std::binary_search(num_prop_list_.begin(), num_prop_list_.end(), prop);
    }
    inline bool isDateProp(const std::string& prop) const
    {
        return std::binary_search(date_prop_list_.begin(), date_prop_list_.end(), prop);
    }

private:
    friend class FMIndexManager;

    typedef std::map<izenelib::util::UString, FilterIdRange> StrIdMapT;
    typedef std::map<NumFilterKeyT, FilterIdRange> NumIdMapT;
    typedef std::vector<StrIdMapT> StrPropIdVecT;
    typedef std::vector<NumIdMapT> NumPropIdVecT;
    typedef std::vector<std::vector<izenelib::util::UString> > PropFilterStrVecT;

    struct PrefixCompare
    {
        PrefixCompare(size_t len)
            : len_(len)
        {
        }

        bool operator()(const StrFilterKeyT& lhs, const StrFilterKeyT& rhs) const
        {
            if (lhs.length() < len_ || rhs.length() < len_)
                return lhs < rhs;

            for (size_t i = 0; i < len_; ++i)
            {
                if (lhs[i] < rhs[i]) return true;
                if (lhs[i] > rhs[i]) return false;
            }

            return false;
        }

        size_t len_;
    };

    void generatePropertyIdForList(const std::vector<std::string>& prop_list, FilterType type);
    NumFilterKeyT formatNumericFilter(size_t prop_id, double filter_num, bool tofloor = true) const;

    void mapGroupFilterToFilterId(
            GroupNode* node,
            const StrFilterItemMapT& group_filter_data,
            StrIdMapT& filterids,
            std::vector<FilterDocListT>& filter_list);
    void mapAttrFilterToFilterId(const StrFilterItemMapT& attr_filter_data, StrIdMapT& filterids, std::vector<FilterDocListT>& filter_list);
    void mapNumericFilterToFilterId(const NumFilterItemMapT& num_filter_data, NumIdMapT& filterids, std::vector<FilterDocListT>& filter_list);

    static void printNode(GroupNode* node, size_t level, const StrIdMapT& filterids, const std::vector<FilterDocListT>& inverted_data)
    {
        if (!node) return;

        for (size_t i = 0; i < level; i++)
            printf("--");
        std::string str;
        node->node_name_.convertString(str, izenelib::util::UString::UTF_8);
        StrIdMapT::const_iterator cit = filterids.find(node->node_name_);
        assert(cit != filterids.end());
        std::cout << str << ", id range ( " << cit->second.start << ","
            << cit->second.end << " ) ";
        if (cit->second.end > cit->second.start)
            std::cout << ", docid size:" << inverted_data[cit->second.start].size() << std::endl;
        else
            std::cout << ", no doc id in current node." << std::endl;
        for (GroupNode::constIter group_cit = node->beginChild();
                group_cit != node->endChild(); ++group_cit)
        {
            printNode(group_cit->second, level + 1, filterids, inverted_data);
        }
    }

    template <class T>
    std::ostream& saveArray_(std::ostream& ofs, const T& arr) const
    {
        size_t len = arr.size();
        ofs.write((const char*)&len, sizeof(len));
        ofs.write((const char*)&arr[0], sizeof(arr[0]) * len);
        return ofs;
    }

    template <class T>
    std::istream& loadArray_(std::istream& ifs, T& arr)
    {
        size_t len = 0;
        ifs.read((char*)&len, sizeof(len));
        arr.resize(len);
        ifs.read((char*)&arr[0], sizeof(arr[0]) * len);
        return ifs;
    }

    faceted::GroupManager* groupManager_;
    faceted::AttrManager* attrManager_;
    NumericPropertyTableBuilder* numericTableBuilder_;
    std::string data_root_path_;

    std::vector<std::string> group_prop_list_;
    std::vector<std::string> attr_prop_list_;
    std::vector<std::string> str_prop_list_;
    std::vector<std::string> date_prop_list_;
    std::vector<std::string> num_prop_list_;

    std::map<std::string, size_t> prop_id_map_;
    std::vector<std::pair<int32_t, std::string> > prop_list_;
    std::vector<int32_t> num_amp_list_;

    StrPropIdVecT str_filter_ids_;
    NumPropIdVecT num_filter_ids_;

    PropFilterStrVecT prop_filter_str_list_;

    std::vector<std::vector<StrFilterKeyT> > str_key_sets_;
    std::vector<std::vector<NumFilterKeyT> > num_key_sets_;

    std::map<std::string, int32_t> num_amp_map_;
    std::vector<std::vector<FilterDocListT> > filter_list_;
    std::set<std::string> unchanged_prop_list_;
    std::vector<StrFilterItemMapT> str_filter_map_;
    bool rebuild_all_;
};

}

#endif
