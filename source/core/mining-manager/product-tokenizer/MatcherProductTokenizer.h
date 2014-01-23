/**
 * @file MatcherProductTokenizer.h
 * @brief product matcher tokenizer.
 */

#ifndef MATCHER_PRODUCT_TOKENIZER_H
#define MATCHER_PRODUCT_TOKENIZER_H

#include "ProductTokenizer.h"

namespace sf1r
{

class MatcherProductTokenizer : public ProductTokenizer
{
public:
    MatcherProductTokenizer() : matcher_(NULL) {}

    virtual void tokenize(ProductTokenParam& param);

    virtual void setProductMatcher(b5m::ProductMatcher* matcher)
    {
        matcher_ = matcher;
    }

private:
    b5m::ProductMatcher* matcher_;
};

}

#endif // MATCHER_PRODUCT_TOKENIZER_H
