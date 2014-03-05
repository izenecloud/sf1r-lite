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

private:
    typedef std::pair<std::string, float> TokenScore;
    typedef std::vector<TokenScore> TokenScoreVec;
    typedef std::map<std::string, float> TokenScoreMap;

    static bool compareTokenScore_(const TokenScore& x, const TokenScore& y);

    void getRefinedResult_(
        const ProductTokenParam::TokenScoreList& majorTokens,
        izenelib::util::UString& refinedResult);
};

}

#endif // PCA_PRODUCT_TOKENIZER_H
