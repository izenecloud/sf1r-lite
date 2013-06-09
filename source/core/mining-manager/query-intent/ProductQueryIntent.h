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
public:
    ProductQueryIntent();
    ~ProductQueryIntent();
public:
    void process(izenelib::driver::Request& request);
    void addClassifier(Classifier* classifier)
    {
        classifiers_.push_back(classifier);
    }
    void reload();
private:
    std::list<Classifier*> classifiers_;
};
}
#endif //ProductQueryIntent.h

