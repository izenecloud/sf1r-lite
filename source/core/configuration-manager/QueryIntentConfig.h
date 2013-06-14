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
        return this->property_ < value.property_;
    }
public:
    std::string property_;
    std::string op_;
};

class QueryIntentConfig;
typedef std::vector<QueryIntentCategory>::iterator QIIterator;

class QueryIntentConfig
{
public:
    void insertQueryIntentConfig(std::string property, std::string op)
    {
        QueryIntentCategory category;
        category.property_ = property;
        category.op_ = op;
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
            if (iCategory.property_ == it->property_)
                return it;
        }
        return intents_.end();
    }
public:
    std::vector<QueryIntentCategory> intents_;
};

}

#endif //QueryIntentConfig.h
