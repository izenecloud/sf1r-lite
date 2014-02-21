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

    void extractMajorTokens_(
        const TokenScoreVec& sortTokens,
        std::vector<std::string>& majorTokens);

    void getMajorTokens_(
        const std::vector<std::string>& majorTokens,
        TokenScoreMap& tokenScoreMap,
        ProductTokenParam::TokenScoreList& tokenScoreList);

    void getMinorTokens_(
        const TokenScoreMap& tokenScoreMap,
        ProductTokenParam::TokenScoreList& tokenScoreList);

    void getRefinedResult_(
        const TokenScoreVec& sortTokens,
        izenelib::util::UString& refinedResult);
};

}

#endif // PCA_PRODUCT_TOKENIZER_H
