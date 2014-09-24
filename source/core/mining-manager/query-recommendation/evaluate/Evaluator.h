#ifndef SF1R_RECOMMEND_EVALUATE_PARSER_H
#define SF1R_RECOMMEND_EVALUATE_PARSER_H

#include <string>
#include <unistd.h>
#include <stdint.h>
#include <list>

namespace sf1r
{
namespace Recommend
{
class EvaluateItem
{
public:
    virtual bool isCorrect(const std::string& result) const = 0;
    virtual const std::string& userQuery() const = 0;
    virtual bool isNeedCorrect() const = 0;
    virtual ~EvaluateItem() {};
};

class Evaluator
{
public:
    class iterator
    {
    public:
        iterator(Evaluator* p = NULL, uint32_t uuid = 0)
            : p_(p)
            , uuid_(uuid)
        {
        }
    
    public:
        const iterator& operator++();
        bool operator== (const iterator& other) const;
        bool operator!= (const iterator& other) const;
        iterator& operator=(const iterator& other);
        const EvaluateItem& operator*() const;
        const EvaluateItem* operator->() const; 
        
        void setEvaluator(Evaluator* p)
        {
            p_ = p;
        }

    private:
        Evaluator* p_;
        uint32_t uuid_;
    };
public:
    Evaluator()
    {
        it_.setEvaluator(this);
    }

    virtual ~Evaluator()
    {
    }
public:
    virtual void load(const std::string& path) = 0;
    virtual bool next() = 0;
    virtual const EvaluateItem& get() const = 0;

    virtual const iterator& begin();
    virtual const iterator& end() const;

    bool isCorrect(std::ostream& out, const std::string& result);
    static void toString(std::string& result);
    static void clear();

private:
    Evaluator::iterator it_;
    static uint32_t nCorrect_;
    static uint32_t nNeed_;
    static uint32_t nNeedRight_;
    static uint32_t nNotNeed_;
    static uint32_t nNotNeedRight_;
};
}
}

#endif
