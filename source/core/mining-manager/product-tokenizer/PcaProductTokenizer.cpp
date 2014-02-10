#include "PcaProductTokenizer.h"
#include <la-manager/TitlePCAWrapper.h>
#include <string>
#include <vector>
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

    for (TokenScoreVec::const_iterator it = tokens.begin();
         it != tokens.end(); ++it)
    {
        scoreSum += it->second;

        if (it->second > maxScore)
        {
            maxScore = it->second;
            maxToken = it->first;
        }
    }

    typedef std::set<std::string> TokenSet;
    TokenSet majorSet;

    if (!brand.empty())
    {
        majorSet.insert(brand);
    }
    if (!model.empty())
    {
        majorSet.insert(model);
    }
    if (!maxToken.empty())
    {
        majorSet.insert(maxToken);
    }

    for (TokenScoreVec::const_iterator it = tokens.begin();
         it != tokens.end(); ++it)
    {
        UString ustr(it->first, UString::UTF_8);
        double score = it->second / scoreSum;
        std::pair<UString, double> tokenScore(ustr, score);

        if (majorSet.find(it->first) != majorSet.end())
        {
            param.majorTokens.push_back(tokenScore);
            if (param.isRefineResult)
            {
                param.refinedResult.append(ustr);
                param.refinedResult.push_back(SPACE_UCHAR);
            }
        }
        else
        {
            param.minorTokens.push_back(tokenScore);
        }
    }
}
