/**
 * @file CMAProductTokenizer.h
 * @brief CMA tokenizer.
 */

#ifndef CMA_PRODUCT_TOKENIZER_H
#define CMA_PRODUCT_TOKENIZER_H

#include "ProductTokenizer.h"
#include <string>

namespace cma
{
class Knowledge;
class Analyzer;
}

namespace sf1r
{

class CMAProductTokenizer : public ProductTokenizer
{
public:
    CMAProductTokenizer(const std::string& dictDir);

    ~CMAProductTokenizer();

    virtual void tokenize(ProductTokenParam& param);

private:
    cma::Knowledge* knowledge_;
    cma::Analyzer* analyzer_;
};

}

#endif // CMA_PRODUCT_TOKENIZER_H
