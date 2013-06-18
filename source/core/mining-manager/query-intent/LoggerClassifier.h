/**
 * @file LoggerClassifier.h
 * @brief 
 * @author Kevin Lin
 * @date Created 2013-06-17
 */
#ifndef SF1R_LOGGER_CLASSIFIER_H
#define SF1R_LOGGER_CLASSIFIER_H

#include "Classifier.h"

namespace sf1r
{

class LoggerClassifier : public Classifier
{
public:
    LoggerClassifier(ClassifierContext* context)
        : Classifier(context)
    {
    }
public:
    bool classify(std::map<QueryIntentCategory, std::list<std::string> >& intents, std::string& query);
    
    const char* name()
    {
        return context_->name_.c_str();
    }
    
    static const char* type()
    {
        return type_;
    }

    int priority()
    {
        return 1;
    }
private:
    static const char* type_;
};

}

#endif //NavieBayesClassifier.h
