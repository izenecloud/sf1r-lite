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
        STR_FILTER = 0,
        NUM_FILTER,

        FILTER_TYPE_COUNT
    };
    struct FilterIdRange
    {
        uint32_t start;
        uint32_t end;
        FilterIdRange()
            :start(0), end(0)
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

    uint32_t loadStrFilterInvertedData(const std::vector<std::string>& property, std::vector<StrFilterItemMapT>& str_filter_data);
    void saveStrFilterInvertedData(const std::vector<std::string>& property, const std::vector<StrFilterItemMapT>& str_filte_data) const;

    void buildGroupFilterData(
            uint32_t last_docid, uint32_t max_docid,
            const std::vector<std::string>& property_list,
            std::vector<StrFilterItemMapT>& group_filter_data);
    void buildAttrFilterData(
            uint32_t last_docid, uint32_t max_docid,
            const std::vector<std::string>& property_list,
            std::vector<StrFilterItemMapT>& attr_filter_data);
    void buildNumericFilterData(
            uint32_t last_docid, uint32_t max_docid,
            const std::vector<std::string>& property_list,
            std::vector<NumFilterItemMapT>& num_filter_data);
    void buildDateFilterData(
            uint32_t last_docid, uint32_t max_docid,
            const std::vector<std::string>& property_list,
            std::vector<NumFilterItemMapT>& date_filter_data);

    izenelib::util::UString FormatGroupPath(const std::vector<izenelib::util::UString>& groupPath) const;
    izenelib::util::UString FormatGroupPath(const std::vector<std::string>& groupPath) const;

    izenelib::util::UString FormatAttrPath(
            const izenelib::util::UString& attrname,
            const izenelib::util::UString& attrvalue) const;
    izenelib::util::UString FormatAttrPath(
            const std::string& attrname,
            const std::string& attrvalue) const;

    FilterIdRange getStrFilterIdRange(size_t prop_id, const izenelib::util::UString& strfilter_key);
    FilterIdRange getNumFilterIdRangeExactly(size_t prop_id, double filter_num) const;
    FilterIdRange getNumFilterIdRangeLarger(size_t prop_id, double filter_num) const;
    FilterIdRange getNumFilterIdRangeSmaller(size_t prop_id, double filter_num) const;

    std::vector<std::vector<FilterDocListT> >& getFilterList();
    void clearFilterList();

    void clearFilterId();
    void saveFilterId();
    void loadFilterId(const std::vector<std::string>& property_list);

    faceted::GroupManager* getGroupManager() const;
    faceted::AttrManager* getAttrManager() const;
    NumericPropertyTableBuilder* getNumericTableBuilder() const;

    void setNumericAmp(const std::map<std::string, int32_t>& num_amp_list);
    const std::map<std::string, int32_t>& getNumericAmp() const;

    izenelib::util::UString getPropFilterString(size_t prop_id, size_t filter_strid) const;
    size_t getMaxPropFilterStrId(size_t prop_id) const;

    size_t getPropertyId(const std::string& property) const;
    size_t propertyCount() const;

private:
    typedef std::map<izenelib::util::UString, FilterIdRange> StrIdMapT;
    typedef std::map<NumFilterKeyT, FilterIdRange> NumIdMapT;
    typedef std::vector<StrIdMapT> StrPropIdVecT;
    typedef std::vector<NumIdMapT> NumPropIdVecT;
    typedef std::vector< std::vector<izenelib::util::UString> >    PropFilterStrVecT;

    FilterIdRange getNumFilterIdRange(size_t prop_id, double filter_num, bool findlarger) const;
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
        cout << str << ", id range ( "<< cit->second.start << ","
            << cit->second.end << " ) ";
        if (cit->second.end > cit->second.start)
            cout << ", docid size:" << inverted_data[cit->second.start].size() << endl;
        else
            cout << ", no doc id in current node." << endl;
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

    std::map<std::string, size_t> prop_id_map_;
    std::vector<std::pair<int, std::string> > prop_list_;
    std::vector<int32_t> num_amp_list_;

    // property -> (GroupPath/Attribute -> filterid)
    StrPropIdVecT strtype_filterids_;
    // property -> (Price/Score -> filterid)
    NumPropIdVecT numtype_filterids_;

    PropFilterStrVecT prop_filterstr_text_list_;

    std::vector<std::vector<NumFilterKeyT> > num_possible_keys_;

    std::map<std::string, int32_t> num_amp_map_;
    std::vector<std::vector<FilterDocListT> > filter_list_;
};

}

#endif
