/**
 * @file ProductQueryIntent.h
 * @brief query intent implement within product.
 * @author Kevin Lin
 * @date Created 2013-06-05
 */

#ifndef SF1R_PRODUCT_QUERY_INTENT_H
#define SF1R_PRODUCT_QUERY_INTENT_H

#include "QueryIntent.h"
#include <vector>

namespace sf1r
{

class ProductQueryIntent : public QueryIntent
{
typedef std::list<Classifier*> ContainerType;
typedef ContainerType::iterator ContainerIterator;
typedef std::vector<ContainerType> PriorityContainer;
public:
    ProductQueryIntent(IntentContext* context);
    ~ProductQueryIntent();
public:
    void process(izenelib::driver::Request& request, izenelib::driver::Response& response);
    
    void addClassifier(Classifier* classifier)
    {
        std::size_t priority = classifier->priority();
        if (priority >= classifiers_.size())
        {
            classifiers_.resize(priority + 1);
        }
        classifiers_[priority].push_back(classifier);
    }
    
    void reloadLexicon();
private:
    PriorityContainer classifiers_;
};

}
#endif //ProductQueryIntent.h

