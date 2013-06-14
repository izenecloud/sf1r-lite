/**
 * @file QueryIntent.h
 * @brief Interface for query intent.
 * @author Kevin Lin
 * @date Created 2013-06-05
 */

#ifndef SF1R_QUERY_INTENT_H
#define SF1R_QUERY_INTENT_H

#include <util/driver/Request.h>
#include "Classifier.h"

namespace sf1r
{

class IntentContext
{
public:
    IntentContext(QueryIntentConfig* config) 
        : config_(config)
    {
    }
public:
    QueryIntentConfig* config_;
};
class QueryIntent
{
public:
    QueryIntent(IntentContext* context)
        : context_(context)
    {
    }
    virtual ~QueryIntent()
    {
        if (context_)
        {
            delete context_;
            context_ = NULL;
        }
    }
public:
    virtual void process(izenelib::driver::Request& request) = 0;
    virtual void addClassifier(Classifier* classifier) = 0;
    virtual void reload() = 0;
protected:
    IntentContext* context_;
};

}
#endif //QueryIntent.h

