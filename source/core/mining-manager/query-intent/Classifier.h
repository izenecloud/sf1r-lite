/**
 * @file Classifier.h
 * @brief Interface for query classify.
 * @author Kevin Lin
 * @date Created 2013-06-05
 */

#ifndef SF1R_CLASSIFIER_H
#define SF1R_CLASSIFIER_H

#include <map>
#include <list>
#include <string>
#include <utility>

#include <configuration-manager/QueryIntentConfig.h>

namespace sf1r
{

class ClassifierContext
{
public:
    ClassifierContext(QueryIntentConfig* config, std::string directory)
        : config_(config)
        , lexiconDirectory_(directory)
    {
    }
public:
    QueryIntentConfig* config_;
    std::string lexiconDirectory_;
};

class Classifier
{
public:
    Classifier(ClassifierContext* context)
        : context_(context)
    {
    }
    virtual ~Classifier()
    {
        if (context_)
        {
            delete context_;
            context_ = NULL;
        }
    }
public:
    //
    // classify query into several QueryIntentType, remove classified keywords from query.
    //
    virtual bool classify(std::map<QueryIntentCategory, std::list<std::string> >& intents, std::string& query) = 0;

    virtual void reload()
    {
    }
protected:
    ClassifierContext* context_;
};

}

#endif //Classifier.h

