/**
 * @file KNlpProductTokenizer.h
 * @brief KNlp tokenizer.
 */

#ifndef KNLP_PRODUCT_TOKENIZER_H
#define KNLP_PRODUCT_TOKENIZER_H

#include "ProductTokenizer.h"

namespace sf1r
{

class KNlpProductTokenizer : public ProductTokenizer
{
public:
    virtual void tokenize(ProductTokenParam& param);

    virtual double sumQueryScore(const std::string& query);

private:
    double tokenizeImpl_(ProductTokenParam& param);

    void getRankBoundary_(ProductTokenParam& param);

};

}

#endif // KNLP_PRODUCT_TOKENIZER_H
