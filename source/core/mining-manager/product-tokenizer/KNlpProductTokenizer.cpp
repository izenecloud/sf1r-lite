#include "KNlpProductTokenizer.h"
#include <common/ResourceManager.h>
#include <common/QueryNormalizer.h>

using namespace sf1r;

void KNlpProductTokenizer::tokenize(ProductTokenParam& param)
{
    tokenizeImpl_(param);
}

double KNlpProductTokenizer::sumQueryScore(const std::string& query)
{
    ProductTokenParam param(query, false);
    return tokenizeImpl_(param);
}

double KNlpProductTokenizer::tokenizeImpl_(ProductTokenParam& param)
{
    const std::string& pattern(param.query);
    ProductTokenParam::TokenScoreList& token_results(param.minorTokens);

    std::string newPattern;
    std::string minor_pattern;
    std::vector<std::string> product_model;
    sf1r::QueryNormalizer::get()->getProductTypes(pattern, product_model, minor_pattern);

    KNlpWrapper::token_score_list_t tokenScores;
    KNlpWrapper::token_score_list_t product_modelScores;
    KNlpWrapper::token_score_list_t minor_patternScore;

    KNlpWrapper::string_t kstr(pattern);
    boost::shared_ptr<KNlpWrapper> knlpWrapper = KNlpResourceManager::getResource();
    knlpWrapper->fmmTokenize(kstr, tokenScores);

    if (!minor_pattern.empty())
    {
        KNlpWrapper::string_t kmstr(minor_pattern);
        knlpWrapper->fmmTokenize(kmstr, minor_patternScore);
    }

    std::vector<KNlpWrapper::string_t> t_product_model;
    for (std::vector<std::string>::iterator i = product_model.begin(); i != product_model.end(); ++i)
    {
        KNlpWrapper::string_t kpmstr(*i);
        t_product_model.push_back(kpmstr);
    }

    double scoreSum = 0;
    KNlpWrapper::token_score_map_t tokenScoreMap;
    for (KNlpWrapper::token_score_list_t::const_iterator it =
             tokenScores.begin(); it != tokenScores.end(); ++it)
        scoreSum += it->second;

    for (KNlpWrapper::token_score_list_t::iterator it =
             minor_patternScore.begin(); it != minor_patternScore.end(); ++it)
    {
        scoreSum -= it->second;
        scoreSum += (it->second/200);
    }

    if (product_model.size() == 1)
    {
        product_modelScores.push_back(std::make_pair(t_product_model[0], scoreSum/5));
    }
    else if (product_model.size() == 2)
    {
        for (std::vector<KNlpWrapper::string_t>::iterator i = t_product_model.begin(); i != t_product_model.end(); ++i)
            product_modelScores.push_back(std::make_pair(*i, scoreSum/8));
    }
    else if (product_model.size() == 3)
    {
        for (std::vector<KNlpWrapper::string_t>::iterator i = t_product_model.begin(); i != t_product_model.end(); ++i)
            product_modelScores.push_back(std::make_pair(*i, scoreSum/12));
    }

    scoreSum = 0;

    /// add all term and its score;
    for (KNlpWrapper::token_score_list_t::iterator it =
             minor_patternScore.begin(); it != minor_patternScore.end(); ++it)
    {
        it->second /= 200;
        tokenScoreMap.insert(*it);
        scoreSum += it->second;
    }

    for (KNlpWrapper::token_score_list_t::const_iterator it =
             product_modelScores.begin(); it != product_modelScores.end(); ++it)
    {
        if(tokenScoreMap.insert(*it).second)
            scoreSum += it->second;
    }

    for (KNlpWrapper::token_score_list_t::const_iterator it =
             tokenScores.begin(); it != tokenScores.end(); ++it)
    {
        if(tokenScoreMap.insert(*it).second)
            scoreSum += it->second;
    }

    //maxScore = 0;
    for (KNlpWrapper::token_score_map_t::const_iterator it =
             tokenScoreMap.begin(); it != tokenScoreMap.end(); ++it)
    {
        std::string str = it->first.get_bytes("utf-8");
        UString ustr(str, UString::UTF_8);
        double score = it->second / scoreSum;
        token_results.push_back(std::make_pair(ustr, score));
    }

    getRankBoundary_(param);

    return scoreSum;
}

void KNlpProductTokenizer::getRankBoundary_(ProductTokenParam& param)
{
    ProductTokenParam::TokenScoreList& minor_tokens(param.minorTokens);
    double& rank_boundary(param.rankBoundary);

    double minor_score_sum = 0;
    double major_score_sum = 0;
    unsigned int minor_size = 0;
    unsigned int major_size = 0;
    unsigned int total_size = 0;
    bool needSmooth = false;
    for (std::list<std::pair<UString, double> >::iterator i = minor_tokens.begin(); i != minor_tokens.end(); ++i)
    {
        if (i->second > 0.1)
        {
            major_score_sum += i->second;
            major_size++;
            if (i->second > 0.5)
            {
                needSmooth = true;
            }
        }
        else
        {
            minor_score_sum += i->second;
            minor_size++;
        }
    }
    total_size = minor_size + major_size;

    if (needSmooth == true || (major_size <= 2 && major_score_sum > 0.6))
    {
        std::vector<double> minor_tokens_point;
        for (std::list<std::pair<UString, double> >::iterator i = minor_tokens.begin(); i != minor_tokens.end(); ++i)
            minor_tokens_point.push_back(i->second);

        KNlpResourceManager::getResource()->gauss_smooth(minor_tokens_point);

        unsigned int k = 0;
        for (std::list<std::pair<UString, double> >::iterator i = minor_tokens.begin();
                                        i != minor_tokens.end(); ++i, k++)
            i->second = minor_tokens_point[k];

        minor_score_sum = 0;
        major_score_sum = 0;
        minor_size = 0;
        major_size = 0;
        total_size = 0;
        for (std::list<std::pair<UString, double> >::iterator i = minor_tokens.begin(); i != minor_tokens.end(); ++i)
        {
            if (i->second > 0.1)
            {
                major_score_sum += i->second;
                major_size++;
            }
            else
            {
                minor_score_sum += i->second;
                minor_size++;
            }
        }
        total_size = minor_size + major_size;
    }

    if (major_size <= 3)
    {
        if (total_size > 8)
        {
            rank_boundary = major_score_sum * 0.85 + minor_score_sum * 0.5;
        }
        else
        {
            rank_boundary = major_score_sum * 0.9 + minor_score_sum * 0.6;
        }
    }
    else if (major_size <= 5)
    {
        if (total_size > 8)
        {
            rank_boundary = major_score_sum * 0.75 + minor_score_sum * 0.5;
        }
        else
        {
            rank_boundary = major_score_sum * 0.8 + minor_score_sum * 0.7;
        }
    }
    else
    {
        if (total_size > 8)
        {
            rank_boundary = major_score_sum * 0.7 + minor_score_sum * 0.5;
        }
        else
        {
            rank_boundary = major_score_sum * 0.7 + minor_score_sum * 0.7;
        }
    }
}

