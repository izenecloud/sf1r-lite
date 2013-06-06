/**
 * @file Classifier.h
 * @brief Interface for query classify.
 * @author Kevin Lin
 * @date Created 2013-06-05
 */

#ifndef SF1R_CLASSIFIER_H
#define SF1R_CLASSIFIER_H

#include <list>
#include <string>
#include <utility>

#include <configuration-manager/QueryIntentConfig.h>

namespace sf1r
{

class Classifier
{
public:
    Classifier(QueryIntentConfig* config)
        : config_(config)
    {
    }
    virtual ~Classifier()
    {
    }
public:
    //
    // classify query into several QueryIntentType, remove classified keywords from query.
    //
    virtual int classify(std::list<std::pair<QueryIntentCategory, std::list<std::string> > >& , std::string& query) = 0;
protected:
    QueryIntentConfig* config_;
};

}

#endif //Classifier.h

