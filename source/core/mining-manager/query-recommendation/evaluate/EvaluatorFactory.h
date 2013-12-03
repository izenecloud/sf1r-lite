#ifndef SF1R_RECOMMEND_EVALUATOR_FACTORY_H
#define SF1R_RECOMMEND_EVALUATOR_FACTORY_H

#include "PositiveEvaluator.h"
#include "NegotiveEvaluator.h"

#include "Evaluator.h"

namespace sf1r
{
namespace Recommend
{
class EvaluatorFactory
{
public: 
    static bool isValid(const std::string& path)
    {
        return PositiveEvaluator::isValid(path) || NegotiveEvaluator::isValid(path);
    }

    static Evaluator* load(const std::string& path)
    {
        if (PositiveEvaluator::isValid(path))
        {
            Evaluator* p = new PositiveEvaluator();
            p->load(path);
            return p;
        }
        if (NegotiveEvaluator::isValid(path))
        {
            Evaluator* p = new NegotiveEvaluator();
            p->load(path);
            return p;
        }
        return NULL;
    }

    static void destory(Evaluator* v)
    {
        if (NULL != v)
        {
            delete v;
            v = NULL;
        }
    }
};
}
}

#endif
