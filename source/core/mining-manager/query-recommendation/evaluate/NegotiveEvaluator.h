#ifndef SF1R_RECOMMEND_NEGOITIVE_EVALUATOR_H
#define SF1R_RECOMMEND_NEGOTIVE_EVALUATOR_H

#include "Evaluator.h"
#include <fstream>
namespace sf1r
{
namespace Recommend
{
class NegotiveEvaluateItem : public EvaluateItem
{
public:
    bool isCorrect(const std::string& result) const
    {
        return "" == result;
    }

    virtual const std::string& userQuery() const
    {
        return userQuery_;
    }
 
    virtual bool isNeedCorrect() const
    {
        return false;
    }
public:
    std::string userQuery_;
};

class NegotiveEvaluator : public Evaluator
{
public:
    NegotiveEvaluator()
        : cLine_(NULL)
        , item_(NULL)
    {
        cLine_ = new char[LINE_SIZE];
        item_ = new NegotiveEvaluateItem();
    }
    ~NegotiveEvaluator()
    {
        if (NULL != cLine_)
        {
            delete[] cLine_;
            cLine_ = NULL;
        }
        if (NULL != item_)
        {
            delete item_;
            item_ = NULL;
        }
    }
public:
    void load(const std::string& path);
    bool next();
    const EvaluateItem& get() const;
    static bool isValid(const std::string& path);

private:
    char* cLine_;
    NegotiveEvaluateItem* item_;
    std::ifstream in_;
    static const int LINE_SIZE;
};
}
}

#endif
