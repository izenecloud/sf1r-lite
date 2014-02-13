/**
 * @file FuzzyTokenNormalizer.h
 * @brief Normalize all of the tokens in fuzzy index and search.
 */

#ifndef FUZZY_TOKEN_NORMALIZER_H
#define FUZZY_TOKEN_NORMALIZER_H

#include "FuzzyNormalizer.h"

namespace sf1r
{
class ProductTokenizer;

class FuzzyTokenNormalizer : public FuzzyNormalizer
{
public:
    FuzzyTokenNormalizer(
        ProductTokenizer* productTokenizer,
        int maxIndexToken);

    virtual void normalizeToken(izenelib::util::UString& token);

    virtual void normalizeText(izenelib::util::UString& text);

private:
    ProductTokenizer* productTokenizer_;

    const int maxIndexToken_;
};

}

#endif // FUZZY_TOKEN_NORMALIZER_H
