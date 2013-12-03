/**
 * @file QueryIntentConfig.h
 * @author Kevin Lin
 * @date Created 2013-06-05
 */
#ifndef SF1R_QUERY_INTENT_CONFIG_H
#define SF1R_QUERY_INTENT_CONFIG_H

#include <string>
#include <vector>
#include <iostream>

namespace sf1r
{

class QueryIntentCategory
{
public:
    bool operator<(const QueryIntentCategory& value) const
    {
        return this->name_ < value.name_;
    }
    QueryIntentCategory& operator=(const QueryIntentCategory& value)
    { 
        name_ = value.name_;
        type_ = value.type_;
        op_ = value.op_;
        operands_ = value.operands_;
        return *this;
    }
public:
    std::string name_;
    std::string type_;
    std::string op_;
    int operands_;
};

class QueryIntentConfig;
typedef std::vector<QueryIntentCategory>::iterator QIIterator;

class QueryIntentConfig
{
public:
    void insertQueryIntentConfig(std::string name, std::string type, std::string op, int operands)
    {
        QueryIntentCategory category;
        category.name_ = name;
        category.type_ = type;
        category.op_ = op;
        category.operands_ = operands;
        intents_.push_back(category);
    }

    QIIterator begin()
    {
        return intents_.begin();
    }
    
    QIIterator end()
    {
        return intents_.end();
    }
    
    QIIterator find(QueryIntentCategory iCategory)
    {
        QIIterator it = intents_.begin();
        for (; it != intents_.end(); it++)
        {
            if (iCategory.name_ == it->name_)
                return it;
        }
        return intents_.end();
    }
public:
    std::vector<QueryIntentCategory> intents_;
};

}

#endif //QueryIntentConfig.h
