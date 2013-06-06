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
    Classifier* createClassifier(ClassifierContext& context);
};

}
#endif //ClassifierFactory.h
