#include "Evaluator.h"
#include <iostream>
#include <stdlib.h>     /* atoi */
#include <stdio.h>

namespace sf1r
{
namespace Recommend
{

uint32_t Evaluator::nCorrect_ = 0;
uint32_t Evaluator::nNeed_ = 0;
uint32_t Evaluator::nNeedRight_ = 0;
uint32_t Evaluator::nNotNeed_ = 0;
uint32_t Evaluator::nNotNeedRight_ = 0;

bool Evaluator::isCorrect(std::ostream& out, const std::string& result)
{
    if ("" != result)
        nCorrect_++;
    if (it_->isNeedCorrect())
    {
        nNeed_++;
        if(it_->isCorrect(result))
        {
            nNeedRight_++;
        }
        else
        {
            out<<it_->userQuery()<<" => "<<result<<"\n";
            return false;
        }
    }
    else
    {
        nNotNeed_++;
        if(it_->isCorrect(result))
        {
            nNotNeedRight_++;
        }
        else
        {
            out<<it_->userQuery()<<" => "<<result<<"\n";
            return false;
        }
    }
    return true;
}
    
void Evaluator::toString(std::string& result)
{
    char tmp[16];
    result += "Total:\t";
    snprintf(tmp, 16, "%u", nNeed_ + nNotNeed_);
    result += tmp;
    result += "\n";
    
    result += "Corrected:\t";
    snprintf(tmp, 16, "%u", nCorrect_);
    result += tmp;
    result += "\n";
    
    result += "Right:\t";
    snprintf(tmp, 16, "%u", (nNeedRight_ + nNotNeedRight_));
    result += tmp;
    result += "\n";
    
    result += "Precision:\t";
    snprintf(tmp, 16, "%f", (double)(nNeedRight_) / nCorrect_);
    result += tmp;
    result += "\n";
    
    result += "Recall:\t";
    snprintf(tmp, 16, "%f", (double) nNeedRight_ / nNeed_);
    result += tmp;
    result += "\n";
    
    result += "Right but Corrected:\t";
    snprintf(tmp, 16, "%f", 1 - (double) nNotNeedRight_ / nNotNeed_);
    result += tmp;
    result += "\n";
}

void Evaluator::clear()
{
    nCorrect_ = 0;
    nNeed_ = 0;
    nNeedRight_ = 0;
    nNotNeed_ = 0;
    nNotNeedRight_ = 0;
}

const Evaluator::iterator& Evaluator::begin()
{
    return ++it_;
}

const Evaluator::iterator& Evaluator::end() const
{
    static Evaluator::iterator it(NULL, -1);
    return it;
}

const Evaluator::iterator& Evaluator::iterator::operator++()
{
    if (p_->next())
    {
        uuid_++;
        return *this;
    }
    else
    {
        uuid_ = -1;
        return *this;
        //return p_->end();
    }
}

const EvaluateItem& Evaluator::iterator::operator*() const
{
    return p_->get();
}

const EvaluateItem* Evaluator::iterator::operator->() const
{
    return &(p_->get());
}

bool Evaluator::iterator::operator== (const Evaluator::iterator& other) const
{
    return uuid_ == other.uuid_;
}

bool Evaluator::iterator::operator!= (const Evaluator::iterator& other) const
{
    return uuid_ != other.uuid_;
}
    
}
}
