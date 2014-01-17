#include "FuzzyAlphaNumNormalizer.h"
#include <util/ustring/algo.hpp>

using namespace sf1r;
using izenelib::util::UString;

void FuzzyAlphaNumNormalizer::normalizeToken(izenelib::util::UString& token)
{
    token = izenelib::util::Algorithm<UString>::padForAlphaNum(token);
}

void FuzzyAlphaNumNormalizer::normalizeText(izenelib::util::UString& text)
{
    normalizeToken(text);
}
