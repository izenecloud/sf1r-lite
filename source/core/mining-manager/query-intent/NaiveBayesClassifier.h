/**
 * @file NaiveBayesClassifier.h
 * @brief 
 * @author Kevin Lin
 * @date Created 2013-06-05
 */
#ifndef SF1R_NAIVE_BAYES_CLASSIFIER_H
#define SF1R_NAIVE_BAYES_CLASSIFIER_H

#include "Classifier.h"

namespace sf1r
{

class NaiveBayesClassifier : public Classifier
{
public:
    NaiveBayesClassifier(ClassifierContext& context)
        : Classifier(context)
    {
    }
public:
    bool classify(std::map<QueryIntentCategory, std::list<std::string> >& intents, std::string& query);
};

}

#endif //NavieBayesClassifier.h
