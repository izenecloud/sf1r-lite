#include <b5m-manager/product_matcher.h>
#include "RemoveKeywords.h"
#include <mining-manager/suffix-match-manager/SuffixMatchManager.hpp>
#include <ctype.h>
#include <limits>
#include "QueryStatistics.h"

namespace sf1r
{
namespace RK
{

std::string INVALID = "";
//#define DEBUG_INFO

typedef bool (*Comparator)(const Token& lv, const Token& rv);

static bool weightComparator(const Token& lv, const Token& rv)
{
    return lv.weight() > rv.weight();
}

static bool locationComparator(const Token& lv, const Token& rv)
{
    return lv.location() > rv.location();
}

static bool tokenComparator(const Token& lv, const Token& rv)
{
    return lv.token() > rv.token();
}

static std::size_t bSearch(const TokenArray& tokens, const Token& token, Comparator comp)
{
    if (tokens.empty())
        return -1;
    std::size_t size = tokens.size();
    std::size_t li = 0;
    std::size_t ri = size - 1;
    while ( li < ri)
    {
        std::size_t mid = li + ((ri - li)>>1);
        if (mid == li || mid == ri)
            break;
        if (comp(tokens[mid],token))
            li = mid;
        else
            ri = mid;
    }
    if (li == ri || li + 1 == ri)
        return ri;
    return -1;
}



static void filter(std::string& keywords)
{
    const char* chars = keywords.c_str();
    std::size_t size = keywords.size();
    for (std::size_t i = 0; i < size; i++)
    {
        if (ispunct(chars[i]) || isdigit(chars[i]))
            keywords.replace(i, 1, " ");
    }
}

static void normalize(TokenArray& tokens)
{
    // Todo outstanding point
    std::size_t size = tokens.size();
    double sum = 0.0;
    std::size_t undefine = 0;
    for (std::size_t i = 0; i < size; i++)
    {
        if (std::numeric_limits<double>::max() == tokens[i].weight())
        {
            undefine ++;
            continue;
        }
        sum += tokens[i].weight();
    }
    if (size == undefine)
    {
        for (std::size_t i = 0; i < size; i++)
        {
            tokens[i].setWeight(1.0 / size);
        }
        return;
    }
    double average = sum / (size - undefine);
    for (std::size_t i = 0; i < size; i++)
    {
        if (std::numeric_limits<double>::max() == tokens[i].weight())
        {
            tokens[i].setWeight(average);
            sum += average;
        }
    }

    sum = 1 / sum;
    for (std::size_t i = 0; i < size; i++)
    {
        tokens[i].scale(sum);
    }
}

void generateTokens(TokenArray& tokens, const std::string& query, MiningManager& miningManager)
{
    std::string keywords = query;
    filter(keywords);
    std::list<std::pair<UString, double> > major_tokens;
    std::list<std::pair<UString, double> > minor_tokens;
    izenelib::util::UString analyzedQuery;
    double rank_boundary = 0;
    if (NULL == miningManager.getSuffixManager())
        return;

    miningManager.getSuffixManager()->GetTokenResults(keywords, major_tokens, minor_tokens, analyzedQuery, rank_boundary);
    //tokens.reserve(major_tokens.size() + minor_tokens.size());
    std::string analyzedString;
    analyzedQuery.convertString(analyzedString, izenelib::util::UString::UTF_8);
    std::cout<<analyzedString<<"\n";
    //std::cout<<keywords<<"\n";
    
    std::list<std::pair<UString, double> >::iterator it = major_tokens.begin();
    for (; it != major_tokens.end(); it++)
    {
        std::string keyword;
        it->first.convertString(keyword, izenelib::util::UString::UTF_8);
        //if (0.01 > it->second)
        //    continue;
        std::size_t pos = keywords.find(keyword);
        if (std::string::npos == pos)
            continue;
        Token token(keyword, it->second, pos);
        
        tokens.push_back(token);
    }
    for (it = minor_tokens.begin(); it != minor_tokens.end(); it++)
    {
        std::string keyword;
        it->first.convertString(keyword, izenelib::util::UString::UTF_8);
        //if (0.01 > it->second)
        //    continue;
        std::size_t pos = keywords.find(keyword);
        if (std::string::npos == pos)
            continue;
        Token token(keyword, it->second, pos);
        
        tokens.push_back(token);
    }
    
    if (tokens.empty() || tokens.size() <= 1)
        return;

    std::sort(tokens.begin(), tokens.end(), locationComparator);
    std::reverse(tokens.begin(), tokens.end());
    // correct location for repeated token
    for (std::size_t i = 0; i < tokens.size() - 1; i++)
    {
        if (tokens[i].location() + tokens[i].token().size() > tokens[i + 1].location())
        {
           tokens[i + 1].setLocation(keywords.find(tokens[i+1].token(), tokens[i+1].location() + 1));
           std::sort(tokens.begin() + i + 1, tokens.end(), locationComparator);
           std::reverse(tokens.begin() + i + 1, tokens.end());
        }
    }

#ifdef DEBUG_INFO    
    std::cout<<"word segment unit::\n";
    std::sort(tokens.begin(), tokens.end(), weightComparator);
    for(std::size_t i = 0; i < tokens.size(); i++)
    {
        std::cout<<tokens[i].token()<<" "<<tokens[i].weight()<<"\n";
    }
#endif
    
    return;    
}

void adjustWeight(TokenArray& tokens, std::string& keywords, MiningManager& miningManager)
{
    if (tokens.empty() || tokens.size() <= 1)
        return;
    
    static ProductMatcher* matcher = ProductMatcherInstance::get();
    if (NULL == matcher)
        return;
    izenelib::util::UString uQuery(keywords, izenelib::util::UString::UTF_8);
    ProductMatcher::KeywordVector kv;
    matcher->ExtractKeywords(uQuery, kv);
   
    std::sort(tokens.begin(), tokens.end(), tokenComparator);
    TokenArray mTokens(tokens); // tokens from matcher
    for (std::size_t i = 0; i < mTokens.size(); i++)
    {
        mTokens[i].setWeight(0.9);
    }

    for (std::size_t i = 0; i< kv.size(); i++)
    {
        std::string keyword;
        kv[i].text.convertString(keyword, izenelib::util::UString::UTF_8);
        
        double w = kv[i].category_name_apps.size() + kv[i].attribute_apps.size();
        std::string attributeName = "";
        if (kv[i].attribute_apps.size() > 0)
        {
            attributeName = kv[i].attribute_apps[0].attribute_name;
        }
        // remove color
        if ("颜色" == attributeName)
        {
            Token token(keyword, 0.0, 0);
            std::size_t found = bSearch(mTokens, token, tokenComparator);
            if (((std::size_t)-1 != found) && (mTokens[found].token() == token.token()))
            {
                tokens[found].setWeight(0.0);
                mTokens[found].setWeight(0.0);
            }
            continue;
        }

        Token token(keyword, w, keywords.find(keyword));
        std::size_t found = bSearch(mTokens, token, tokenComparator);
        if (((std::size_t)-1 != found) && (mTokens[found].token() == token.token()))
        {
            if (w > 10)
            {
                tokens[found].setCenterToken(true);
                mTokens[found].setWeight(std::numeric_limits<double>::max());
            }
            else
                mTokens[found].setWeight(w);
        }
    }
    
    normalize(mTokens);
    
#ifdef DEBUG_INFO    
    std::cout<<"word from product matcher::\n";
    for (std::size_t i = 0; i < mTokens.size(); i++)
    {
        std::cout<<mTokens[i].token()<<" "<<mTokens[i].weight()<<"\n";
    }
#endif

    double factor = 1.5;
    for (std::size_t i = 0; i < tokens.size(); i++)
    {
        tokens[i].combine(mTokens[i].weight() * factor);
    }
    mTokens.clear();
    
#ifdef DEBUG_INFO    
    std::cout<<"tuned after product_matcher::\n";
    normalize(tokens);
    std::sort(tokens.begin(), tokens.end(), weightComparator);
    for (std::size_t i = 0; i < tokens.size(); i++)
    {
        std::cout<<tokens[i].token()<<" "<<tokens[i].weight()<<"\n";
    }
#endif
    
    std::sort(tokens.begin(), tokens.end(), locationComparator);
    std::reverse(tokens.begin(), tokens.end());
    
    QueryStatistics* qs = miningManager.getQueryStatistics();
    if (NULL == qs)
        return;

    TokenArray tokenFreqs(tokens);
    for (std::size_t i = 0; i < tokenFreqs.size(); i++)
    {
        double f = qs->frequency(tokenFreqs[i].token());
        tokenFreqs[i].setWeight(f);
    }
    
    // combine via frequency
    for (std::size_t i = 0; i <tokenFreqs.size() - 1; i++)
    {
        if (qs->isCombine(tokenFreqs[i].token(), tokenFreqs[i+1].token()))
        {
            tokenFreqs[i] += tokenFreqs[i+1];
            tokenFreqs.erase(tokenFreqs.begin() + i + 1);
            tokens[i] += tokens[i+1];
            tokens.erase(tokens.begin() + i + 1);
        }
    }
    
    normalize(tokenFreqs);
    bool reNormalize = false;
    for (std::size_t i = 0; i < tokenFreqs.size(); i++)
    {
        if (tokenFreqs[i].weight() > 0.8)
        {
            tokens[i].setCenterToken(true);
            tokenFreqs[i].setWeight(std::numeric_limits<double>::max());
            reNormalize = true;
            break;
        }
    }
    if (reNormalize)
        normalize(tokenFreqs);
#ifdef DEBUG_INFO    
    std::cout<<"freq::\n";
    for (std::size_t i = 0; i < tokenFreqs.size(); i++)
    {
        std::cout<<tokenFreqs[i].token()<<"  "<<tokenFreqs[i].weight()<<"\n";
    }
#endif
    factor = 1.5;
    //cin>>factor;
    for (std::size_t i = 0; i < tokens.size(); i++)
    {
        double f = tokenFreqs[i].weight();
        //tokens[i].scale(factor * f);
        tokens[i].combine(factor * f);
    }
    tokenFreqs.clear();
#ifdef DEBUG_INFO    
    std::cout<<"tuned after statistical::\n";
    normalize(tokens);
    std::sort(tokens.begin(), tokens.end(), weightComparator);
    for (std::size_t i = 0; i < tokens.size(); i++)
    {
        std::cout<<tokens[i].token()<<"  "<<tokens[i].weight()<<" "<<tokens[i].isCenterToken()<<"\n";
    }
#endif

    normalize(tokens);
}

static void tagCenterTokens(TokenArray& tokens)
{
    double hWeight = 0.0;
    for (std::size_t i = 0; i < tokens.size(); i++)
    {
        if (tokens[i].isCenterToken())
            continue;
        if (hWeight < tokens[i].weight())
            hWeight = tokens[i].weight();
    }
    hWeight += 0.01;
    for (std::size_t i = 0; i < tokens.size(); i++)
    {
        if (tokens[i].isCenterToken())
            tokens[i].setWeight(hWeight);
    }
    normalize(tokens);
   
    std::sort(tokens.begin(), tokens.end(), weightComparator);
    double weight = 0.0;
    double threshold = 0.95;
    std::size_t maxCTs = tokens.size() * 0.75; 
    for (std::size_t i = 0; i < tokens.size(); i++)
    {
        if ((tokens[i].weight() < 0.01) || (i >= maxCTs))
            break;
        tokens[i].setCenterToken(true);
        weight += tokens[i].weight();
        if ((weight >= threshold))
        {
            break;
        }
    }
}


static void expandCenterTokens(TokenArray& tokens)
{
    std::sort(tokens.begin(), tokens.end(), locationComparator);
    
    TokenArray exCT(tokens);
    for (std::size_t i = 0; i < tokens.size(); i++)
    {
        if (tokens[i].isCenterToken())
        {
            if ((i > 0) && tokens[i].isLeft(tokens[i-1]))
                exCT[i-1].setCenterToken(true);
            if ((i < tokens.size() - 1) && tokens[i].isRight(tokens[i+1]))
            {
                exCT[i+1].setCenterToken(true);
            }
        }
    }
    
    for (std::size_t i = 0; i < exCT.size(); i++)
    {
        if (exCT[i].isCenterToken())
            exCT[i].combine(1.0);
    }
    tokens.clear();
    tokens.swap(exCT);
    return;
    
    /*
    for (std::size_t i = 0; i < exCT.size(); i++)
    {
        if (exCT[i].isCenterToken())
            tokens.push_back(exCT[i]);
    }*/
}

static void recommandTokens(TokenArray& tokens, TokenRecommended& tokensRecommended)
{
    std::sort(tokens.begin(), tokens.end(), weightComparator);
    std::size_t centerTokenNum = 0;
    std::size_t size = tokens.size();
    for (std::size_t i = 0; i < size; i++)
    {
        if (tokens[i].isCenterToken())
            continue;
        centerTokenNum = i;
        break;
    }

    double threshold = 0.2;
    threshold *= size;
    threshold -= 1;
    Token reserve("", 0.0, 0);
    TokenArray leveledTokens;
    for (std::size_t i = 0; i < centerTokenNum - 1; i++)
    {
        leveledTokens.clear();
        if (i >= threshold)
        {
            for (std::size_t j = i; j < size; j++)
            {
                leveledTokens.push_back(reserve + tokens[j]);
            }
            tokensRecommended.push_back(leveledTokens);
        }
        reserve += tokens[i];
    }
    tokensRecommended.sort();
    // Maybe drop two word at one time if too long and too low weight
    //for (std::size_t i = 0; i < tokens.size(); i++)
    //{
    //    query += tokens[i];
    //    if (i >= threshold)
    //        queries.push_back(query);
    //}
    //std::sort(queries.begin(), queries.end(), weightComparator);
    tokens.clear();
}

static void removeMultiTokens(TokenArray& tokens, TokenRecommended& queries)
{
#ifdef DEBUG_INFO    
    std::cout<<"Tokens::\n";
    for (std::size_t i = 0; i < tokens.size(); i++)
    {
        std::cout<<i<<": "<<tokens[i].token()<<" "<<tokens[i].weight()<<"\n";
    }
#endif
    //First step: center word
    tagCenterTokens(tokens);
#ifdef DEBUG_INFO    
    std::cout<<"Center Tokens::\n";
    for (std::size_t i = 0; i < tokens.size(); i++)
    {
        if (tokens[i].isCenterToken())
            std::cout<<i<<": "<<tokens[i].token()<<" "<<tokens[i].weight()<<"\n";
    }
#endif
    //Second step: expand center word
    expandCenterTokens(tokens);
#ifdef DEBUG_INFO    
    std::cout<<"Expaned::\n";
    for(std::size_t i = 0; i < tokens.size(); i++)
    {
        if (tokens[i].isCenterToken())
            std::cout<<tokens[i].token()<<" "<<tokens[i].weight()<<"\n";
    }
#endif

    //Third step: 
    recommandTokens(tokens, queries);
}

void removeTokens(TokenArray& tokens, TokenRecommended& queries)
{
    if (2 == tokens.size())
    {
        std::sort(tokens.begin(), tokens.end(), weightComparator);
        TokenArray tk;
        tk.push_back(tokens[0] + tokens[1]);
        queries.push_back(tk);
        tk.clear();
        
        tk.push_back(tokens[0]);
        queries.push_back(tk);
        tk.clear();

        tk.push_back(tokens[1]);
        queries.push_back(tk);
        return;
    }
    else if ( 3 == tokens.size())
    {
        for (std::size_t i = 0; i < tokens.size(); i++)
        {
            if (tokens[i].isCenterToken())
                tokens[i].combine(1.0);
        }
        std::sort(tokens.begin(), tokens.end(), weightComparator);
        TokenArray tk;
        tk.push_back(tokens[0] + tokens[1] + tokens[2]);
        queries.push_back(tk);
        tk.clear();
        
        tk.push_back(tokens[0] + tokens[1]);
        queries.push_back(tk);
        tk.clear();
        
        tk.push_back(tokens[0]);
        queries.push_back(tk);
        return;
    }
    else if ( 4 <= tokens.size())
    {
        removeMultiTokens(tokens, queries);
    }
}

void queryAbbreviation(TokenRecommended& queries, std::string& keywords, MiningManager& miningManager)
{
    TokenArray tokens;
    generateTokens(tokens, keywords, miningManager);
    adjustWeight(tokens, keywords, miningManager);
    removeTokens(tokens, queries);
    std::cout<<"recommand::\n";
    std::cout<<queries;
}

}
}
