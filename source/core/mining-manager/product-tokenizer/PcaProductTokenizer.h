/**
 * @file PcaProductTokenizer.h
 * @brief PCA tokenizer.
 */

#ifndef PCA_PRODUCT_TOKENIZER_H
#define PCA_PRODUCT_TOKENIZER_H

#include "ProductTokenizer.h"

namespace sf1r
{

class PcaProductTokenizer : public ProductTokenizer
{
public:
    virtual void tokenize(ProductTokenParam& param);
};

}

#endif // PCA_PRODUCT_TOKENIZER_H
