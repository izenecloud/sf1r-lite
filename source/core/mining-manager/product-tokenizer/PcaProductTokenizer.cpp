#include "PcaProductTokenizer.h"
#include <la-manager/TitlePCAWrapper.h>
#include <glog/logging.h>
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
const float kTokensThreshold = 0.5;
}

void PcaProductTokenizer::normalize_(const TokenScoreVec& tokens, TokenScoreVec& sortTokens)
{
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

    for (TokenScoreMap::iterator it = tokenScoreMap.begin();
         it != tokenScoreMap.end(); ++it)
    {
        it->second /= scoreSum;
        sortTokens.push_back(*it);
    }
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

    TokenScoreVec sortTokens;
    normalize_(tokens, sortTokens);

    std::sort(sortTokens.begin(), sortTokens.end(), compareTokenScore_);
    const std::size_t refineNum = std::min(sortTokens.size(), kMajorTermNum);
    if (param.isRefineResult)
    {
        for (unsigned int i = 0; i < refineNum; ++i)
        {
            UString ustr(sortTokens[i].first, UString::UTF_8);
            param.refinedResult.append(ustr);
            param.refinedResult.push_back(SPACE_UCHAR);   
        }
        //getRefinedResult_(refindedQuery, param.refinedResult);
    }
    
    if (param.useFuzzyThreshold)
    {
        std::vector<std::pair<UString, double> > majorTokens;
        std::vector<std::pair<UString, double> > minorTokens;
        LOG(INFO) << "Use Fuzzy Threshold: " << param.fuzzyThreshold << "; Major Tokens Threshold: " << param.tokensThreshold;
        float Score = 0;
        for (size_t i = 0; i < sortTokens.size(); ++i)
        {
            UString ustr(sortTokens[i].first, UString::UTF_8);
            std::pair<UString, double> tokenScore(ustr, sortTokens[i].second);
            if (Score < param.tokensThreshold)
                majorTokens.push_back(tokenScore);
            else
                minorTokens.push_back(tokenScore);

            Score += sortTokens[i].second;
            if (Score > param.fuzzyThreshold)
                break;
        }

        for (std::vector<std::pair<UString, double> >::iterator i = majorTokens.begin();
             i != majorTokens.end(); ++i)
        {
            std::pair<UString, double> tokenScore(i->first, (i->second)/Score);
            param.majorTokens.push_back(tokenScore);
        }

        for (std::vector<std::pair<UString, double> >::iterator i = minorTokens.begin();
             i != minorTokens.end(); ++i)
        {
            std::pair<UString, double> tokenScore(i->first, (i->second)/Score);
            param.minorTokens.push_back(tokenScore);        
        }
    }
    else
    {
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
    }

    //add 
    if (param.usePrivilegeQuery && !param.privilegeQuery.empty())
    {
        LOG(INFO) << "Use privilege Query : " << param.privilegeQuery << " ; privilege weight : " << param.privilegeWeight;
        TokenScoreVec tokens_p, subTokens_p;
        std::string brand;
        std::string model;
        titlePCA->pca(param.privilegeQuery, tokens_p, brand, model, subTokens_p, false);
    
        TokenScoreVec sortTokens_p;
        normalize_(tokens_p, sortTokens_p);

        for (size_t i = 0; i < sortTokens_p.size(); ++i)
        {
            UString ustr(sortTokens_p[i].first, UString::UTF_8);
            std::pair<UString, double> tokenScore(ustr, sortTokens_p[i].second * param.privilegeWeight);
            param.minorTokens.push_back(tokenScore);
        }
    }
    //param.rankBoundary = 0.5;
    param.rankBoundary = 0.0;
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
