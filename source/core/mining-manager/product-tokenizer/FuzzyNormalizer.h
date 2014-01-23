/**
 * @file FuzzyNormalizer.h
 * @brief Normalize the token and text, for improving fuzzy search accuracy.
 */

#ifndef FUZZY_NORMALIZER_H
#define FUZZY_NORMALIZER_H

#include <util/ustring/UString.h>

namespace sf1r
{

class FuzzyNormalizer
{
public:
    virtual ~FuzzyNormalizer() {}

    /**
     * Normalize the @p token for query.
     */
    virtual void normalizeToken(izenelib::util::UString& token) = 0;

    /**
     * Normalize the @p text for index.
     */
    virtual void normalizeText(izenelib::util::UString& text) = 0;
};

}

#endif // FUZZY_NORMALIZER_H
