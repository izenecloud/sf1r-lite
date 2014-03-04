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

    std::vector<std::string> majorTokens;
    extractMajorTokens_(sortTokens, majorTokens);

    getMajorTokens_(majorTokens, tokenScoreMap, param.majorTokens);
    getMinorTokens_(tokenScoreMap, param.minorTokens);

    param.rankBoundary = 0.5;

    if (param.isRefineResult)
    {
        getRefinedResult_(majorTokens, param.refinedResult);
    }
}

bool PcaProductTokenizer::compareTokenScore_(const TokenScore& x, const TokenScore& y)
{
    return x.second > y.second;
}

void PcaProductTokenizer::extractMajorTokens_(
    const TokenScoreVec& sortTokens,
    std::vector<std::string>& majorTokens)
{
    const std::size_t num = std::min(sortTokens.size(), kMajorTermNum);

    for (size_t i = 0; i < num; ++i)
    {
        majorTokens.push_back(sortTokens[i].first);
    }
}

void PcaProductTokenizer::getMajorTokens_(
    const std::vector<std::string>& majorTokens,
    TokenScoreMap& tokenScoreMap,
    ProductTokenParam::TokenScoreList& tokenScoreList)
{
    for (std::vector<std::string>::const_iterator it = majorTokens.begin();
         it != majorTokens.end(); ++it)
    {
        TokenScoreMap::iterator mapIter = tokenScoreMap.find(*it);

        if (mapIter == tokenScoreMap.end())
            continue;

        UString ustr(mapIter->first, UString::UTF_8);
        std::pair<UString, double> tokenScore(ustr, mapIter->second);

        tokenScoreMap.erase(mapIter);
        tokenScoreList.push_back(tokenScore);
    }
}

void PcaProductTokenizer::getMinorTokens_(
    const TokenScoreMap& tokenScoreMap,
    ProductTokenParam::TokenScoreList& tokenScoreList)
{
    for (TokenScoreMap::const_iterator it = tokenScoreMap.begin();
         it != tokenScoreMap.end(); ++it)
    {
        UString ustr(it->first, UString::UTF_8);
        std::pair<UString, double> tokenScore(ustr, it->second);

        tokenScoreList.push_back(tokenScore);
    }
}

void PcaProductTokenizer::getRefinedResult_(
    const std::vector<std::string>& majorTokens,
    izenelib::util::UString& refinedResult)
{
    for (std::vector<std::string>::const_iterator it = majorTokens.begin();
         it != majorTokens.end(); ++it)
    {
        UString ustr(*it, UString::UTF_8);
        refinedResult.append(ustr);
        refinedResult.push_back(SPACE_UCHAR);
    }
}
