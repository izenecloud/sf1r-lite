/**
 * @file Classifier.h
 * @brief Interface for query classify.
 * @author Kevin Lin
 * @date Created 2013-06-05
 */

#ifndef SF1R_CLASSIFIER_H
#define SF1R_CLASSIFIER_H

#include "QueryIntentType.h"

#include <map>
#include <list>
#include <string>

namespace sf1r
{

class Classifier
{
public:
    virtual ~Classifier()
    {
    }
public:
    //
    // classify query into several QueryIntentType, remove classified keywords from query.
    //
    virtual int classify(std::map<QueryIntentType, std::list<std::string> >& , std::string& query) = 0;
};

}

#endif //Classifier.h

