#include "PcaProductTokenizer.h"
#include <la-manager/TitlePCAWrapper.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>

using namespace sf1r;
using izenelib::util::UString;

namespace
{
const std::size_t kMajorTermNum = 3;
}

void PcaProductTokenizer::tokenize(ProductTokenParam& param)
{
    TokenScoreVec tokens, subTokens;
    std::string brand;
    std::string model;

    TitlePCAWrapper* titlePCA = TitlePCAWrapper::get();
    titlePCA->pca(param.query, tokens, brand, model, subTokens, false);

    if (tokens.empty())
        return;

    TokenScoreMap tokenScoreMap;
    double scoreSum = 0;

    for (TokenScoreVec::const_iterator it = tokens.begin();
         it != tokens.end(); ++it)
    {
        if (tokenScoreMap.find(it->first) != tokenScoreMap.end())
            continue;

        tokenScoreMap[it->first] = it->second;
        scoreSum += it->second;
    }

    TokenScoreVec sortTokens;
    for (TokenScoreMap::iterator it = tokenScoreMap.begin();
         it != tokenScoreMap.end(); ++it)
    {
        it->second /= scoreSum;
        sortTokens.push_back(*it);
    }

    std::sort(sortTokens.begin(), sortTokens.end(), compareTokenScore_);

    const std::size_t majorNum = std::min(sortTokens.size(), kMajorTermNum);
    for (size_t i = 0; i < majorNum; ++i)
    {
        UString ustr(sortTokens[i].first, UString::UTF_8);
        std::pair<UString, double> tokenScore(ustr, sortTokens[i].second);
        param.majorTokens.push_back(tokenScore);
    }

    for (size_t i = majorNum; i < sortTokens.size(); ++i)
    {
        UString ustr(sortTokens[i].first, UString::UTF_8);
        std::pair<UString, double> tokenScore(ustr, sortTokens[i].second);
        param.minorTokens.push_back(tokenScore);
    }

    param.rankBoundary = 0.5;

    if (param.isRefineResult)
    {
        getRefinedResult_(param.majorTokens, param.refinedResult);
    }
}

bool PcaProductTokenizer::compareTokenScore_(const TokenScore& x, const TokenScore& y)
{
    if (x.second == y.second)
        return x.first > y.first;

    return x.second > y.second;
}

void PcaProductTokenizer::getRefinedResult_(
    const ProductTokenParam::TokenScoreList& majorTokens,
    izenelib::util::UString& refinedResult)
{
    for (ProductTokenParam::TokenScoreList::const_iterator it =
                 majorTokens.begin(); it != majorTokens.end(); ++it)
    {
        refinedResult.append(it->first);
        refinedResult.push_back(SPACE_UCHAR);
    }
}
