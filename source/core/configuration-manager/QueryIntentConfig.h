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
public:
    std::string name_;
    std::string property_;
    std::string op_;
};

class QueryIntentConfig;
typedef std::vector<QueryIntentCategory>::iterator QIIterator;

class QueryIntentConfig
{
public:
    void insertQueryIntentConfig(std::string name,
                std::string property, std::string op)
    {
        QueryIntentCategory category;
        category.name_ = name;
        category.property_ = property;
        category.op_ = op;
        intents_.push_back(category);
    }

    int getQueryIntentCategory(QueryIntentCategory& intentCategory)
    {
        int size = intents_.size();
        for (int i= 0; i < size; i++)
        {
            if (intents_[i].name_ == intentCategory.name_)
            {
                intentCategory.property_ = intents_[i].property_;
                intentCategory.op_ = intents_[i].op_;
                return 0;
            }
        }
        return -1;
    }
    
    QIIterator begin()
    {
        return intents_.begin();
    }
    
    QIIterator end()
    {
        return intents_.end();
    }
public:
    std::vector<QueryIntentCategory> intents_;
};

}

#endif //QueryIntentConfig.h
