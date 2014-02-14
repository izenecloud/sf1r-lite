#include "PcaProductTokenizer.h"
#include <la-manager/TitlePCAWrapper.h>
#include <string>
#include <vector>
#include <map>
#include <set>

using namespace sf1r;
using izenelib::util::UString;

void PcaProductTokenizer::tokenize(ProductTokenParam& param)
{
    typedef std::vector<std::pair<std::string, float> > TokenScoreVec;
    TokenScoreVec tokens, subTokens;
    std::string brand;
    std::string model;

    TitlePCAWrapper* titlePCA = TitlePCAWrapper::get();
    titlePCA->pca(param.query, tokens, brand, model, subTokens, false);

    double scoreSum = 0;
    double maxScore = 0;
    std::string maxToken;

    typedef std::map<std::string, float> TokenScoreMap;
    TokenScoreMap tokenScoreMap;

    for (TokenScoreVec::const_iterator it = tokens.begin();
         it != tokens.end(); ++it)
    {
        if (tokenScoreMap.find(it->first) != tokenScoreMap.end())
            continue;

        tokenScoreMap[it->first] = it->second;
        scoreSum += it->second;

        if (it->second > maxScore)
        {
            maxScore = it->second;
            maxToken = it->first;
        }
    }

    std::vector<std::string> majorTokens;
    if (!maxToken.empty() &&
        maxToken != brand && maxToken != model)
    {
        majorTokens.push_back(maxToken);
    }
    if (!brand.empty())
    {
        majorTokens.push_back(brand);
    }
    if (!model.empty())
    {
        majorTokens.push_back(model);
    }

    for (std::vector<std::string>::const_iterator it = majorTokens.begin();
         it != majorTokens.end(); ++it)
    {
        TokenScoreMap::iterator mapIter = tokenScoreMap.find(*it);

        if (mapIter == tokenScoreMap.end())
            continue;

        UString ustr(mapIter->first, UString::UTF_8);
        double score = mapIter->second / scoreSum;
        std::pair<UString, double> tokenScore(ustr, score);

        tokenScoreMap.erase(mapIter);
        param.majorTokens.push_back(tokenScore);

        if (param.isRefineResult)
        {
            param.refinedResult.append(ustr);
            param.refinedResult.push_back(SPACE_UCHAR);
        }
    }

    for (TokenScoreMap::const_iterator it = tokenScoreMap.begin();
         it != tokenScoreMap.end(); ++it)
    {
        UString ustr(it->first, UString::UTF_8);
        double score = it->second / scoreSum;
        std::pair<UString, double> tokenScore(ustr, score);

        param.minorTokens.push_back(tokenScore);
    }
}
