/**
 * @file ProductQueryIntent.h
 * @brief query intent implement within product.
 * @author Kevin Lin
 * @date Created 2013-06-05
 */

#ifndef SF1R_PRODUCT_QUERY_INTENT_H
#define SF1R_PRODUCT_QUERY_INTENT_H

#include "QueryIntent.h"

namespace sf1r
{
class ProductQueryIntent : public QueryIntent
{
public:
    ~ProductQueryIntent()
    {
    }
public:
    void process(izenelib::driver::Request& request);
    void setClassifier(Classifier* classifier)
    {
        classifier_ = classifier;
    }
};
}
#endif //ProductQueryIntent.h

