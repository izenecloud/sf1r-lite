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
#include "ConditionParser.h"

#define MAXSIZE 5


namespace sf1r {
using namespace izenelib::driver;

class ConditionsNode
{
public:
    ConditionsNode()
    {
        relation_ = "and";  // the default relation is "and";
    }

    ~ConditionsNode()
    {
    }

    bool isLeafNode()
    {
        return conditionLeafList_.size() != 0 && pConditionNodeList_.size() == 0;
    }

    unsigned int getChildNum()
    {
        return conditionLeafList_.size() + pConditionNodeList_.size();
    }

    bool setRelation(const std::string relation);
    std::string getRelation();
    void addConditionLeaf(const ConditionParser& condParser);
    void addConditionNode(const boost::shared_ptr<ConditionsNode> pCondNode);


    std::vector<ConditionParser>& getConditionLeafList()
    {
        return conditionLeafList_;
    }
    std::vector<boost::shared_ptr<ConditionsNode> >& getConditionNodeList()
    {
        return pConditionNodeList_;
    }

private:
    std::string relation_;
    std::vector<ConditionParser> conditionLeafList_;
    std::vector<boost::shared_ptr<ConditionsNode> > pConditionNodeList_;
};

class ConditionsTree
{
public:
    ConditionsTree();

    ~ConditionsTree()
    {
    }
    void init(boost::shared_ptr<ConditionsNode> proot);

    bool isEmpty()
    {
        return isempty_;
    }

    void setRoot(boost::shared_ptr<ConditionsNode> proot);

    boost::shared_ptr<ConditionsNode>& getRoot();

    void addConditionLeaf(boost::shared_ptr<ConditionsNode>& pCurrentCondNode
        , const ConditionParser& condParser);

    void addConditionNode(boost::shared_ptr<ConditionsNode>& pCurrentCondNode
        , boost::shared_ptr<ConditionsNode> pCondNode);

    void printConditionsTree(boost::shared_ptr<ConditionsNode> pNode);

private:
    boost::shared_ptr<ConditionsNode> root_;
    bool isempty_;
};

}

#endif //SF1R_DRIVER_PARSERS_CONDITION_NODE_H