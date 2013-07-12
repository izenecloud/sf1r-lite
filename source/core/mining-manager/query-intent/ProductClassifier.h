/**
 * @file ProductClassifier.h
 * @brief 
 * @author Kevin Lin
 * @date Created 2013-06-i13
 */
#ifndef SF1R_PRODUCT_CLASSIFIER_H
#define SF1R_PRODUCT_CLASSIFIER_H

#include "Classifier.h"
#include "../product-classifier/SPUProductClassifier.hpp"

namespace sf1r
{

class ProductClassifier : public Classifier
{
public:
    ProductClassifier(ClassifierContext* context)
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
    static const char* type_;
    NQI::KeyTypePtr keyPtr_;
};

}

#endif //NavieBayesClassifier.h
