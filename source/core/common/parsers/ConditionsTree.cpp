#include "ConditionsTree.h" 

namespace sf1r {
/*
bool ConditionsNode::setRelation(const std::string relation)
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
        return false;

    return true;
}

std::string ConditionsNode::getRelation()
{
    return relation_;
}
void ConditionsNode::addConditionLeaf(const ConditionParser& condParser)
{
    conditionLeafList_.push_back(condParser);
}

void ConditionsNode::addConditionNode(const boost::shared_ptr<ConditionsNode> pCondNode)
{
    pConditionNodeList_.push_back(pCondNode);
}

// ConditionTree 
ConditionsTree::ConditionsTree()
{
    isempty_ = true;
}

void ConditionsTree::init(boost::shared_ptr<ConditionsNode> proot)
{
    root_ = proot;
    isempty_ = false;
}

void ConditionsTree::setRoot(boost::shared_ptr<ConditionsNode> proot)
{
    root_ = proot;
    isempty_ = false;
}

boost::shared_ptr<ConditionsNode>& ConditionsTree::getRoot()
{
    return root_;
}

void ConditionsTree::addConditionLeaf(boost::shared_ptr<ConditionsNode>& pCurrentCondNode
        , const ConditionParser& condParser)
{
    pCurrentCondNode->addConditionLeaf(condParser);
}

void ConditionsTree::addConditionNode(boost::shared_ptr<ConditionsNode>& pCurrentCondNode
        , boost::shared_ptr<ConditionsNode> pCondNode)
{
    pCurrentCondNode->addConditionNode(pCondNode);
}

void ConditionsTree::printConditionsTree(boost::shared_ptr<ConditionsNode> pNode)
{
}
*/
}