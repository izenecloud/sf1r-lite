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

class QueryIntent
{
public:
    QueryIntent(Classifier* classifier)
        :classifier_(classifier)
    {
    }
    QueryIntent()
        :classifier_(NULL)
    {
    }
    virtual ~QueryIntent()
    {
    }
public:
    virtual void process(izenelib::driver::Request& request) = 0;
    virtual void setClassifier(Classifier* classifier) = 0;
public:
    Classifier* classifier_;
};

}
#endif //QueryIntent.h

