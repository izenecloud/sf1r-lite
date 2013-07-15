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
        NQI::KeyType name;
        name.name_ = context_->name_;
        keyPtr_ = context_->config_->find(name);
    }
public:
    bool classify(NQI::WMVContainer& wmvs, std::string& query);
    
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
    NQI::KeyTypePtr keyPtr_;
    static const char* type_;
};

}

#endif //NavieBayesClassifier.h
