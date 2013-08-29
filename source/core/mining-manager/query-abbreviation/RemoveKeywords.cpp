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

#define DEBUG_INFO

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

void generateTokens(TokenArray& tokens, std::string& keywords, MiningManager& miningManager)
{
    filter(keywords);
    
    std::list<std::pair<UString, double> > major_tokens;
    std::list<std::pair<UString, double> > minor_tokens;
    izenelib::util::UString analyzedQuery;
    double rank_boundary = 0;
    miningManager.getSuffixManager()->GetTokenResults(keywords, major_tokens, minor_tokens, analyzedQuery, rank_boundary);
    tokens.reserve(major_tokens.size() + minor_tokens.size());
    
    std::list<std::pair<UString, double> >::iterator it = major_tokens.begin();
    for (; it != major_tokens.end(); it++)
    {
        std::string keyword;
        it->first.convertString(keyword, izenelib::util::UString::UTF_8);
        if (0.01 > it->second)
            continue;
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
        if (0.01 > it->second)
            continue;
        std::size_t pos = keywords.find(keyword);
        if (std::string::npos == pos)
            continue;
        Token token(keyword, it->second, pos);
        
        tokens.push_back(token);
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
    static ProductMatcher* matcher = ProductMatcherInstance::get();
    izenelib::util::UString uQuery(keywords, izenelib::util::UString::UTF_8);
    ProductMatcher::KeywordVector kv;
    matcher->ExtractKeywords(uQuery, kv);
    
    
    TokenArray mTokens(tokens); // tokens from matcher
    for (std::size_t i = 0; i < mTokens.size(); i++)
    {
        mTokens[i].setWeight(0.9);
    }
    std::sort(tokens.begin(), tokens.end(), tokenComparator);
    for (std::size_t i = 0; i< kv.size(); i++)
    {
        std::string keyword;
        kv[i].text.convertString(keyword, izenelib::util::UString::UTF_8);
        double w = kv[i].category_name_apps.size() + kv[i].attribute_apps.size();
        Token token(keyword, w, 0);
        std::size_t found = bSearch(mTokens, token, tokenComparator);
        if (((std::size_t)-1 != found) && (mTokens[found].token() == token.token()))
        {
            if ((w > 10) && (kv[i].IsModel() || kv[i].IsBrand()))
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

    double factor = 1.0;
    for (std::size_t i = 0; i < tokens.size(); i++)
    {
        //tokens[i].scale(mTokens[i].weight() * factor);
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

    static QueryStatistics* qs = miningManager.getQueryStatistics();
    TokenArray tokenFreqs(tokens);
    for (std::size_t i = 0; i < tokenFreqs.size(); i++)
    {
        double f = qs->frequency(tokenFreqs[i].token());
        tokenFreqs[i].setWeight(f);
    }
    normalize(tokenFreqs);
#ifdef DEBUG_INFO    
    std::cout<<"freq::\n";
    for (std::size_t i = 0; i < tokenFreqs.size(); i++)
    {
        std::cout<<tokenFreqs[i].token()<<"  "<<tokenFreqs[i].weight()<<"\n";
    }
#endif
    
    factor = 1.0;
    for (std::size_t i = 0; i < tokens.size(); i++)
    {
        double f = tokenFreqs[i].weight();
        //tokens[i].scale(factor * f);
        tokens[i].combine(factor * f);
    }
    tokenFreqs.clear();
#ifdef DEBUG_INFO    
    std::cout<<"tuned after statistical::\n";
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
    double threshold = 0.8;
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
    
    tokens.clear();
    for (std::size_t i = 0; i < exCT.size(); i++)
    {
        if (exCT[i].isCenterToken())
            tokens.push_back(exCT[i]);
    }
}

static void recommandTokens(TokenArray& tokens, TokenArray& queries)
{
    std::sort(tokens.begin(), tokens.end(), weightComparator);
    Token query("", 0.0, 0);
    double threshold = 0.25;
    threshold *= tokens.size();
    threshold -= 1;
    // Maybe drop two word at one time if too long and too low weight
    for (std::size_t i = 0; i < tokens.size(); i++)
    {
        query = query + tokens[i];
        if (i >= threshold)
            queries.push_back(query);
    }
    std::sort(queries.begin(), queries.end(), tokenComparator);
    tokens.clear();
}

static void removeMultiTokens(TokenArray& tokens, TokenArray& queries)
{
#ifdef DEBUG_INFO    
    std::cout<<"Tokens::\n";
    for (std::size_t i = 0; i < tokens.size(); i++)
    {
        std::cout<<i<<": "<<tokens[i].token()<<" "<<tokens[i].weight()<<" "<<tokens[i].location()<<"\n";
    }
#endif
    //First step: center word
    tagCenterTokens(tokens);
#ifdef DEBUG_INFO    
    std::cout<<"Center Tokens::\n";
    for (std::size_t i = 0; i < tokens.size(); i++)
    {
        if (tokens[i].isCenterToken())
            std::cout<<i<<": "<<tokens[i].token()<<" "<<tokens[i].weight()<<" "<<tokens[i].location()<<"\n";
    }
#endif
    //Second step: expand center word
    expandCenterTokens(tokens);
#ifdef DEBUG_INFO    
    std::cout<<"Expaned::\n";
    for(std::size_t i = 0; i < tokens.size(); i++)
    {
        std::cout<<tokens[i].token()<<" "<<tokens[i].weight()<<" "<<tokens[i].location()<<"\n";
    }
#endif

    //Third step: 
    recommandTokens(tokens, queries);
    std::cout<<"recommand::\n";
    for(std::size_t i = 0; i < queries.size(); i++)
    {
        std::cout<<queries[i].token()<<"\n";
    }
}

void removeTokens(TokenArray& tokens, TokenArray& queries)
{
    
    if (2 == tokens.size())
    {
        std::sort(tokens.begin(), tokens.end(), tokenComparator);
        queries.push_back(tokens[0]);
        queries.push_back(tokens[1]);
        return;
    }
    if ( 3 == tokens.size())
    {
        std::sort(tokens.begin(), tokens.end(), tokenComparator);
        std::string query = tokens[0].token() + tokens[1].token();
        Token token(query, tokens[0].weight() + tokens[1].weight(), -1);
        queries.push_back(token); 
        queries.push_back(tokens[0]);
        return;
    }
    if ( 4 <= tokens.size())
    {
        removeMultiTokens(tokens, queries);
    }
}

}
}
