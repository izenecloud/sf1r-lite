#include "ConditionsTree.h" 

namespace sf1r {

bool conditonEqual(const ConditionsNode& conditionNode1,
    const ConditionsNode& conditionNode2)

{
    // if (conditionNode1 == NULL || conditionNode2 == NULL)
    // {
    //     return true;
    // }

    // if (conditionNode1.relation_ != conditionNode2.relation_
    //         || conditionNode1.conditionLeafList_ != conditionNode2.conditionLeafList_
    //         || conditionNode1.pConditionsNodeList_.size() != conditionNode2.pConditionsNodeList_.size())
    //         return false;

    // std::vector<boost::shared_ptr<ConditionsNode> >::iterator iter1 = conditionNode1.pConditionsNodeList_.begin();
    // std::vector<boost::shared_ptr<ConditionsNode> >::iterator iterEnd1 = conditionNode1.pConditionsNodeList_.end();

    // std::vector<boost::shared_ptr<ConditionsNode> >::iterator iter2 = conditionNode2.pConditionsNodeList_.begin();
    // std::vector<boost::shared_ptr<ConditionsNode> >::iterator iterEnd2 = conditionNode2.pConditionsNodeList_.end();

    // for (; iter1 != iterEnd1 && iter2 != iterEnd2; ++iter1, ++iter2)
    // {
    //     if (! conditonEqual(*iter1, *iter2))
    //     {
    //         return false;
    //     }
    // }

    return true;
}
}