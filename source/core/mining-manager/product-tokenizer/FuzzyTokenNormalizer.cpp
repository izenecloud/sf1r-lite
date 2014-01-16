#include "FuzzyTokenNormalizer.h"
#include "ProductTokenizer.h"
#include <util/ustring/algo.hpp>

using namespace sf1r;
using izenelib::util::UString;

namespace
{
const UString::CharT kUCharSpace = ' ';
}

FuzzyTokenNormalizer::FuzzyTokenNormalizer(
    ProductTokenizer* productTokenizer,
    int maxIndexToken)
        : productTokenizer_(productTokenizer)
        , maxIndexToken_(maxIndexToken)
{
}

void FuzzyTokenNormalizer::normalizeToken(izenelib::util::UString& token)
{
    UString result;
    result.reserve(token.size()); // byte number

    result += kUCharSpace;
    izenelib::util::Algorithm<UString>::padForAlphaNumCompact(token, result);
    result += kUCharSpace;

    token.swap(result);
}

void FuzzyTokenNormalizer::normalizeText(izenelib::util::UString& text)
{
    if (text.empty())
        return;

    UString result;
    result.reserve(text.size()); // byte number
    result += kUCharSpace;

    std::string textUtf8;
    text.convertString(textUtf8, UString::UTF_8);

    ProductTokenParam tokenParam(textUtf8, false);
    productTokenizer_->tokenize(tokenParam);

    int tokenCount = 0;

    for (ProductTokenParam::TokenScoreListIter it = tokenParam.majorTokens.begin();
         it != tokenParam.majorTokens.end(); ++it)
    {
        if (maxIndexToken_ > 0 && tokenCount >= maxIndexToken_)
            break;

        result += it->first;
        result += kUCharSpace;
        ++tokenCount;
    }

    for (ProductTokenParam::TokenScoreListIter it = tokenParam.minorTokens.begin();
         it != tokenParam.minorTokens.end(); ++it)
    {
        if (maxIndexToken_ > 0 && tokenCount >= maxIndexToken_)
            break;

        result += it->first;
        result += kUCharSpace;
        ++tokenCount;
    }

    text.swap(result);
}
