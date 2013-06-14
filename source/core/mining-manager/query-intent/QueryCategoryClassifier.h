/**
 * @file QueryClassifier.h
 * @brief 
 * @author Kevin Lin
 * @date Created 2013-06-i13
 */
#ifndef SF1R_QUERY_CATEGORY_CLASSIFIER_H
#define SF1R_QUERY_CATEGORY_CLASSIFIER_H

#include "Classifier.h"
#include "../product-classifier/SPUProductClassifier.hpp"

namespace sf1r
{

class QueryCategoryClassifier : public Classifier
{
public:
    QueryCategoryClassifier(ClassifierContext* context)
        : Classifier(context)
    {
    }
public:
    bool classify(std::map<QueryIntentCategory, std::list<std::string> >& intents, std::string& query);
    const char* name()
    {
        return name_;
    }
    static const char* type()
    {
        return name_;
    }
private:
    static const char* name_;
};

}

#endif //NavieBayesClassifier.h
