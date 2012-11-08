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
        ChildContainerT::iterator it = child_nodes_.begin();
        while(it != child_nodes_.end())
        {
            delete it->second;
            ++it;
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
        if(child)
        {
            if(child_nodes_.find(child->node_name_) != child_nodes_.end())
                return false;
            child_nodes_[child->node_name_] = child;
            return true;
        }
        return false;
    }
    bool appendChild(const izenelib::util::UString& node_name)
    {
        if(child_nodes_.find(node_name) != child_nodes_.end())
            return false;
        GroupNode* n = new GroupNode(node_name);
        child_nodes_[node_name] = n;
        return true;
    }
    bool removeChild(const izenelib::util::UString& node_name)
    {
        ChildContainerT::iterator it = child_nodes_.find(node_name);
        if(it == child_nodes_.end())
            return false;
        delete it->second;
        child_nodes_.erase(it);
    }
    GroupNode* getChild(const izenelib::util::UString& node_name)
    {
        ChildContainerT::iterator it = child_nodes_.find(node_name);
        if(it == child_nodes_.end())
            return NULL;
        return it->second;
    }
};


class FilterManager
{
public:
    enum FilterType
    {
        StrFilter = 0,
        NumFilter,

        LastTypeFilter
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

    FilterIdRange getStrFilterIdRange(const std::string& property, const izenelib::util::UString& strfilter_key);

    void buildGroupFilterData(
            uint32_t last_docid, uint32_t max_docid,
            const std::vector< std::string >& propertys,
            std::vector<StrFilterItemMapT>& group_filter_data);
    void buildAttrFilterData(
            uint32_t last_docid, uint32_t max_docid,
            const std::vector<std::string>& propertys,
            std::vector<StrFilterItemMapT>& attr_filter_data);
    void buildNumberFilterData(
            uint32_t last_docid, uint32_t max_docid,
            const std::vector< std::string >& propertys,
            std::vector<NumFilterItemMapT>& num_filter_data);

    izenelib::util::UString FormatGroupPath(const std::vector<izenelib::util::UString>& groupPath) const;
    izenelib::util::UString FormatGroupPath(const std::vector<std::string>& groupPath) const;

    izenelib::util::UString FormatAttrPath(
            const izenelib::util::UString& attrname,
            const izenelib::util::UString& attrvalue) const;
    izenelib::util::UString FormatAttrPath(
            const std::string& attrname,
            const std::string& attrvalue) const;

    FilterIdRange getNumFilterIdRangeExactly(const std::string& property, float filter_num) const;
    FilterIdRange getNumFilterIdRangeLarger(const std::string& property, float filter_num) const;
    FilterIdRange getNumFilterIdRangeSmaller(const std::string& property, float filter_num) const;

    std::vector<FilterDocListT>& getAllFilterInvertedData();
    void clearAllFilterInvertedData();

    void clearFilterId();
    void saveFilterId();
    void loadFilterId(const std::vector<std::string> propertys);

    faceted::GroupManager* getGroupManager() const;
    faceted::AttrManager* getAttrManager() const;
    NumericPropertyTableBuilder* getNumericTableBuilder() const;

private:
    typedef std::map<izenelib::util::UString, FilterIdRange> StrIdMapT;
    typedef std::map<NumFilterKeyT, FilterIdRange> NumberIdMapT;
    typedef std::map<std::string, StrIdMapT> StrPropertyIdMapT;
    typedef std::map<std::string, NumberIdMapT> NumPropertyIdMapT;

    FilterIdRange getNumFilterIdRange(const std::string& property, float filter_num, bool findlarger) const;
    NumFilterKeyT formatNumberFilter(const std::string& property, float filter_num, bool tofloor = true) const;
    void mapGroupFilterToFilterId(
            GroupNode* node,
            const StrFilterItemMapT& group_filter_data,
            StrIdMapT& filterids);
    void mapAttrFilterToFilterId(const StrFilterItemMapT& attr_filter_data, StrIdMapT& filterids);
    void mapNumberFilterToFilterId(const NumFilterItemMapT& num_filter_data, NumberIdMapT& filterids);
    static void printNode(GroupNode* node, size_t level, const StrIdMapT& filterids, const std::vector<FilterDocListT>& inverted_data)
    {
        if(node)
        {
            for(size_t i = 0; i < level; i++)
                printf("--");
            std::string str;
            node->node_name_.convertString(str, izenelib::util::UString::UTF_8);
            StrIdMapT::const_iterator cit = filterids.find(node->node_name_);
            assert(cit != filterids.end());
            cout << str << ", id range ( "<< cit->second.start << ","
                << cit->second.end << " ) ";
            if(cit->second.end > cit->second.start)
                cout << ", docid size:" << inverted_data[cit->second.start].size() << endl;
            else
                cout << ", no doc id in current node." << endl;
            GroupNode::constIter group_cit = node->beginChild();
            while(group_cit != node->endChild())
            {
                printNode(group_cit->second, level + 1, filterids, inverted_data);
                ++group_cit;
            }
        }
    }

    faceted::GroupManager* groupManager_;
    faceted::AttrManager* attrManager_;
    NumericPropertyTableBuilder* numericTableBuilder_;
    std::string data_root_path_;
    // property=>(GroupPath/Attribute =>filterid)
    StrPropertyIdMapT  strtype_filterids_;

    // property=>(Price/Score=>filterid)
    NumPropertyIdMapT numbertype_filterids_;
    std::map<std::string, std::vector<NumFilterKeyT> > num_possible_keys_;

    std::vector< FilterDocListT > all_inverted_filter_data_;
};

}

#endif
