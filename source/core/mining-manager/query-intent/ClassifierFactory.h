/**
 * @file ClassifierFactory.h
 * @brief 
 * @author Kevin Lin
 * @date Created 2013-06-05
 */

#ifndef SF1R_CLASSIFIER_FACTORY_H
#define SF1R_CLASSIFIER_FACTORY_H

#include "Classifier.h"

namespace sf1r
{

class ClassifierContext
{
};

class ClassifierFactory
{
public:
    ClassifierFactory(QueryIntentConfig* config)
        : config_(config)
    {
    }
public:
    Classifier* createClassifier(ClassifierContext& context);
private:
    QueryIntentConfig* config_;
};

}
#endif //ClassifierFactory.h
