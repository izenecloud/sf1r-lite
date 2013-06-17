#ifndef _SF1R_FILTERING_TREE_
#define _SF1R_FILTERING_TREE_ 
/**
 * @file core/query-manager/FilteringTree.h
 * @author Hongliang Zhao
 * @date Created <2013-05-16>
 */

namespace sf1r{

class FilteringTree
{
public:
    FilteringTree();
    ~FilteringTree();

    unsigned int size() const
    {
        return filteringTreeList_.size();
    }

    const QueryFiltering::FilteringTreeValue& getFilteringTreeValue(unsigned int i) const
    {
        return filteringTreeList_[i];
    }

    FilteringTree& operator=(const FilteringTree& obj)
    {
        filteringTreeList_ = obj.filteringTreeList_;
        return (*this);
    }

    bool operator==(const FilteringTree& obj) const
    {
        return filteringTreeList_ == obj.filteringTreeList_;
    }

private:
    std::vector<QueryFiltering::FilteringTreeValue> filteringTreeList_;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & filteringTreeList_;
    }
};

}

#endif ///_SF1R_FILTERING_TREE_

