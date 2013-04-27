
#include "ConditionTreeParser.h"
#include <boost/algorithm/string/case_conv.hpp>
#include <vector>
#include <common/Keys.h>
#include <iostream>
using namespace std;
namespace sf1r {

    using driver::Keys;
    
    bool ConditionTreeParser::parse(const Value& conditions) //{}
    {
        clearMessages();

        if (conditions.type() == Value::kNullType)
        {
            return true;
        }

        if (conditions.type() != Value::kObjectType)
        {
            error() = "Condition must be an object.";
            return false;
        }

        Value conditionArray; //[]
        std::string relation;
        conditionArray = conditions[Keys::condition_array];
        relation = asString(conditions[Keys::relation]);

        // here deal with two situaions 
        if (conditionsTree_->isEmpty())
        {
            boost::shared_ptr<ConditionsNode> proot(new ConditionsNode());
            conditionsTree_->setRoot(proot);
            ConditionsNodeStack_.push(proot);
        }

        //get current need to deal node
        boost::shared_ptr<ConditionsNode> pCurrentNode = ConditionsNodeStack_.top();
        pCurrentNode->setRelation(relation);

        //check if the object is leafe or non-leafe;
        const Value::ArrayType* cond_array = conditionArray.getPtr<Value::ArrayType>();
        if (!cond_array)
        {
            error() = "Conditions must be an array";
            return false;
        }

        for (unsigned int i = 0; i < cond_array->size(); ++i)
        {
            std::string property_ = asString(((*cond_array)[i])[Keys::property]); // if, this is right ... 
            if (property_.empty())
            {
                boost::shared_ptr<ConditionsNode> pChildNode(new ConditionsNode());
                ConditionsNodeStack_.push(pChildNode);
                conditionsTree_->addConditionNode(pCurrentNode, pChildNode);
                parse((*cond_array)[i]);
            }
            else
            {
                ConditionParser condition;
                condition.parse((*cond_array)[i]);
                conditionsTree_->addConditionLeaf(pCurrentNode, condition);
            }
        }

        ConditionsNodeStack_.pop();
        return true;
    }
}
