/**
 * @file ProductQueryIntent.h
 * @brief query intent implement within product.
 * @author Kevin Lin
 * @date Created 2013-06-05
 */

#ifndef SF1R_PRODUCT_QUERY_INTENT_H
#define SF1R_PRODUCT_QUERY_INTENT_H

#include "QueryIntent.h"
#include <list>

namespace sf1r
{
class ProductQueryIntent : public QueryIntent
{
typedef std::map<std::string, std::pair<bool, Classifier*> > ContainerType;
typedef ContainerType::iterator ContainerIterator;
public:
    ProductQueryIntent(IntentContext* context);
    ~ProductQueryIntent();
public:
    void process(izenelib::driver::Request& request);
    void addClassifier(Classifier* classifier)
    {
        classifiers_[classifier->name()] = (make_pair(true,classifier));
    }
    void reload();
private:
    ContainerType classifiers_;
};
}
#endif //ProductQueryIntent.h

