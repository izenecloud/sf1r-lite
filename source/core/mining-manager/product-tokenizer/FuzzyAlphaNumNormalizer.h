/**
 * @file FuzzyAlphaNumNormalizer.h
 * @brief Normalize the alphabets and numbers.
 */

#ifndef FUZZY_ALPHA_NUM_NORMALIZER_H
#define FUZZY_ALPHA_NUM_NORMALIZER_H

#include "FuzzyNormalizer.h"

namespace sf1r
{

class FuzzyAlphaNumNormalizer : public FuzzyNormalizer
{
public:
    virtual void normalizeToken(izenelib::util::UString& token);

    virtual void normalizeText(izenelib::util::UString& text);
};

}

#endif // FUZZY_ALPHA_NUM_NORMALIZER_H
