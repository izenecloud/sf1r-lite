#ifndef SF1R_DRIVER_PARSERS_CONDITION_NODE_H
#define SF1R_DRIVER_PARSERS_CONDITION_NODE_H
/**
 * @file common/parsers/ConditionsTree.h
 * @author Hongliang Zhao
 * @date Created <2013-04-24>
 */
#include <string>
#include <queue>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <query-manager/QueryTypeDef.h>
#include <glog/logging.h>
#include <common/sf1_msgpack_serialization_types.h>
#include <3rdparty/msgpack/msgpack.hpp>
#include <util/izene_serialization.h>
#include <net/aggregator/Util.h>


//using namespace izenelib::driver;
namespace sf1r {

///this is node with relatonship, the leaf node is FilteringType;
class ConditionsNode
{
public:
    std::string relation_;
    
    std::vector<QueryFiltering::FilteringType> conditionLeafList_;
    
    std::vector<ConditionsNode> conditionsNodeList_;

    ConditionsNode()
    : relation_("and")
    {}

    bool isLeafNode()
    {
        return conditionLeafList_.size() != 0 && conditionsNodeList_.size() == 0;
    }

    unsigned int getChildNum()
    {
        return conditionLeafList_.size() + conditionsNodeList_.size();
    }

    bool setRelation(const std::string relation)
    {
        if (relation == "AND" || relation == "and")
        {
            relation_ = "and";
        }
        else if (relation == "OR" || relation == "or")
        {
            relation_ = "or";
        }
        else
        {
            LOG(ERROR) << "THE Relation must be \"AND\" or \"and\" or  \"OR\" or \"or\" ..."<< std::endl;
            return false;
        }

        return true;
    }

    bool empty()
    {
        if (conditionLeafList_.size() == 0 && conditionsNodeList_.size() == 0)
            return true;
        return false;
    }

    bool getFilteringListSuffix(std::vector<QueryFiltering::FilteringType>& filteringRules)
    {
        if (relation_ != "and" || conditionsNodeList_.size() != 0)
            return false;

        for (std::vector<QueryFiltering::FilteringType>::iterator i = conditionLeafList_.begin();
                i != conditionLeafList_.end(); ++i)
        {
            filteringRules.push_back(*i);   
        }
        return true;
    }

    void printConditionInfo()
    {
        LOG(INFO) << ": relation:" << relation_;
        std::cout << "LeafCondition number: " << conditionLeafList_.size() << std::endl;
        std::cout << "NodeCondition number: " << conditionsNodeList_.size() << std::endl;
    }

    ConditionsNode& operator=(const ConditionsNode& obj)
    {
        relation_ = obj.relation_;
        conditionLeafList_ = obj.conditionLeafList_;
        conditionsNodeList_ = obj.conditionsNodeList_;
        return (*this);
    }

    bool operator==(const ConditionsNode& obj) const
    {
        return relation_ == obj.relation_
        && conditionLeafList_ == obj.conditionLeafList_
        && conditionsNodeList_ == obj.conditionsNodeList_;
    }

    DATA_IO_LOAD_SAVE(ConditionsNode, & relation_ & conditionLeafList_ & conditionsNodeList_);

    MSGPACK_DEFINE(relation_, conditionLeafList_, conditionsNodeList_);

private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & relation_;
        ar & conditionLeafList_;
        ar & conditionsNodeList_;
    }
};

bool conditonEqual(const ConditionsNode& conditionNode1,
    const ConditionsNode& conditionNode2);

}

#endif //SF1R_DRIVER_PARSERS_CONDITION_NODE_H