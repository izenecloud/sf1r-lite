#ifndef POSITIVE_EVALUATOR_H
#define POSITIVE_EVALUATOR_H

#include "Evaluator.h"
#include <fstream>
#include <iostream>


namespace sf1r
{
namespace Recommend
{
class PositiveEvaluateItem : public EvaluateItem
{
public:
    bool isCorrect(const std::string& result) const
    {
        return correct_ == result;
    }

    virtual const std::string& userQuery() const
    {
        return userQuery_;
    }
    
    virtual bool isNeedCorrect() const
    {
        return true;
    }
 
public:
    std::string userQuery_;
    std::string correct_;
};

class PositiveEvaluator : public Evaluator
{
public:
    PositiveEvaluator()
        : cLine_(NULL)
        , item_(NULL)
    {
        cLine_ = new char[LINE_SIZE];
        item_ = new PositiveEvaluateItem();
    }
    ~PositiveEvaluator()
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
    PositiveEvaluateItem* item_;
    std::ifstream in_;
    static const int LINE_SIZE;
};
}
}

#endif
