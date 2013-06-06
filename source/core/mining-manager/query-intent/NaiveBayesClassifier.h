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
    int classify(std::map<QueryIntentType, std::list<std::string> >& , std::string& query);
};

}

#endif //NavieBayesClassifier.h
