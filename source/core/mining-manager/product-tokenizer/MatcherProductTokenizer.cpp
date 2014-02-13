#include "MatcherProductTokenizer.h"
#include <b5m-manager/product_matcher.h>

using namespace sf1r;
using izenelib::util::UString;

void MatcherProductTokenizer::tokenize(ProductTokenParam& param)
{
    if (!matcher_)
        return;

    UString patternUStr(param.query, UString::UTF_8);
    std::list<UString> left;

    ProductTokenParam::TokenScoreList& majorTokens = param.majorTokens;
    ProductTokenParam::TokenScoreList& minorTokens = param.minorTokens;
    matcher_->GetSearchKeywords(patternUStr, majorTokens, minorTokens, left);

    if (!majorTokens.empty())
    {
        ProductTokenParam::TokenScoreList::iterator it = majorTokens.begin();
        while (it != majorTokens.end())
        {
            if (patternUStr.find(it->first) != UString::npos)
            {
                if (param.isRefineResult)
                {
                    param.refinedResult.append(it->first);
                    param.refinedResult.push_back(SPACE_UCHAR);
                }
                ++it;
            }
            else
            {
                it = majorTokens.erase(it);
            }
        }
    }

    std::list<std::pair<UString, double> > left_tokens;
    if (getBigramTokens_(left, left_tokens, 0.1))
    {
        minorTokens.splice(minorTokens.end(), left_tokens);
    }
}
