#include "RemoveKeywords.h"
#include <mining-manager/suffix-match-manager/SuffixMatchManager.hpp>
#include <ctype.h>
#include "QueryStatistics.h"

namespace sf1r
{
namespace RK
{

static bool tokenComparator(const Token& lv, const Token& rv)
{
    return lv.weight() > rv.weight();
}

static bool locationComparator(const Token& lv, const Token& rv)
{
    return lv.location() < rv.location();
}

static inline bool isChinesePunct(int c)
{
    //if (('，' == c) || ('。' == c) || ('；' == c) || ('！' == c) || ('“' == c)
    //    ('”' == c) || ('：' == c) || ('？' == c) || ('（ == c) || ('）' == c))
        return true;
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

void generateTokens(TokenArray& tokens, std::string& keywords, MiningManager& miningManager)
{
    filter(keywords);
    
    std::list<std::pair<UString, double> > major_tokens;
    std::list<std::pair<UString, double> > minor_tokens;
    izenelib::util::UString analyzedQuery;
    double rank_boundary = 0;
    miningManager.getSuffixManager()->GetTokenResults(keywords, major_tokens, minor_tokens, analyzedQuery, rank_boundary);
    tokens.reserve(major_tokens.size() + minor_tokens.size());
    
    std::string analyzedQueryString;
    analyzedQuery.convertString(analyzedQueryString, izenelib::util::UString::UTF_8);
    std::cout<<"AnalyzedQuery:"<<analyzedQueryString<<"\n";

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
        
        pos = analyzedQueryString.find(keyword);
        if (std::string::npos != pos)
            token.scale(2.0);    
        
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
        
        pos = analyzedQueryString.find(keyword);
        if (std::string::npos != pos)
            token.scale(2.0);    
        
        tokens.push_back(token);
    }
        
    std::sort(tokens.begin(), tokens.end(), locationComparator);
    return;    
}

void adjustWeight(TokenArray& tokens, MiningManager& miningManager)
{
    std::cout<<"before scale::\n";
    for(std::size_t i = 0; i < tokens.size(); i++)
    {
        std::cout<<tokens[i].token()<<" "<<tokens[i].weight()<<" "<<tokens[i].location()<<"\n";
    }
    
    static QueryStatistics* qs = miningManager.getQueryStatistics();
    std::cout<<"freq::\n";
    for (std::size_t i = 0; i < tokens.size(); i++)
    {
        double f = qs->frequency(tokens[i].token());
        tokens[i].scale(f + 0.001);
        std::cout<<tokens[i].token()<<"  "<<f<<"\n";
    }
}

//static void centerWords(TokenArray& tokens, TokenArray& centerWords)
static void tagCenterTokens(TokenArray& tokens)
{
    std::sort(tokens.begin(), tokens.end(), tokenComparator);
    double weight = 0.0;
    double threshold = 0.7;
    double sum = 0.0;
    for (std::size_t i = 0; i < tokens.size(); i++)
    {
        sum += tokens[i].weight();
    }
    threshold *= sum;

    for (std::size_t i = 0; i < tokens.size(); i++)
    {
        tokens[i].setCenterToken(true);
        weight += tokens[i].weight();
        if (weight >= threshold)
        {
            break;
        }
    }
}

static std::size_t bSearch(const TokenArray& tokens, const Token& token)
{
    std::size_t location = token.location();
    std::size_t size = tokens.size();
    std::size_t li = 0;
    std::size_t ri = size - 1;
    while ( li <= ri)
    {
        std::size_t mid = li + ((ri - li)>>1);
        std::size_t midLocation = tokens[mid].location();
        //std::cout<<mid<<": "<<midLocation<<" "<<location<<"\n";
        if (location == midLocation)
            return mid;
        else if (location < midLocation)
        {
            ri = mid - 1;
        }
        else
        {
            li = mid + 1;
        }
    }
    return -1;
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
        if (exCT[i].isCenterToken() && (0.01 < exCT[i].weight()))
            tokens.push_back(exCT[i]);
    }
            

    /*
    std::sort(centerTokens.begin(), centerTokens.end(), locationComparator);
    
    std::cout<<"center words::\n";
    for (std::size_t i = 0; i < centerTokens.size(); i++)
    {
        std::cout<<centerTokens[i].token()<<" "<<centerTokens[i].weight()<<" "<<centerTokens[i].location()<<"\n";
    }
    
    std::size_t lastIndex = -1;
   
    std::sort(tokens.begin(), tokens.end(), locationComparator);
    std::cout<<"tokens::\n";
    for(std::size_t i = 0; i < tokens.size(); i++)
    {
        std::cout<<i<<": "<<tokens[i].token()<<" "<<tokens[i].weight()<<" "<<tokens[i].location()<<"\n";
    }
    
    for (std::size_t i = 0; i < centerTokens.size(); i++)
    {
        std::size_t index = bSearch(tokens, centerTokens[i]);
        if ( (std::size_t)-1 == index)
            continue;
        
        std::cout<<centerTokens[i].token()<<":"<<centerTokens[i].location()<<" == "<<tokens[index].token()<<":"<<tokens[index].location()<<"\n";
        // left
        if ((index  > 0) && (tokens[index-1].location() + tokens[index-1].token().size() == tokens[index].location()))
        {
            if (-1 == lastIndex)
            {
                if (index - 1 > 0)
                    tokens.erase(tokens.begin(), tokens.begin() + index - 1);
                lastIndex = index;
            }
            else
            {
                if (index - 1 > lastIndex + 1)
                    tokens.erase(tokens.begin() + lastIndex + 1, tokens.begin() + index - 1);
                lastIndex += 2;
            }
        }
        else
        {
            if (-1 == lastIndex)
            {
                if (index > 0)
                    tokens.erase(tokens.begin(), tokens.begin() + index);
                lastIndex = index;
            }
            else
            {
                if (index > lastIndex + 1)
                    tokens.erase(tokens.begin() + lastIndex + 1, tokens.begin() + index);
                lastIndex ++;
            }
        }

        // right
        std::cout<<tokens[lastIndex].token()<<"\n";
        std::cout<<tokens[lastIndex].location() + tokens[lastIndex].token().size()<<" "<<tokens[lastIndex+1].location()<<"\n";
        if (tokens[lastIndex].location() + tokens[lastIndex].token().size() == tokens[lastIndex+1].location())
        {
            lastIndex ++;
        }
    }
    if (lastIndex < tokens.size())
        tokens.erase(tokens.begin() + lastIndex + 1, tokens.end());
    centerTokens.clear();

    for (std::size_t i = 0; i < tokens.size(); i++)
    {
        if (0.01 > tokens[i].weight())
            tokens.erase(tokens.begin() + i);
    }*/
}

static void recommandTokens(TokenArray& tokens, TokenArray& queries)
{
    std::sort(tokens.begin(), tokens.end(), tokenComparator);
    Token query("", 0.0, 0);
    for (std::size_t i = 0; i < tokens.size(); i++)
    {
        query = query + tokens[i];
        queries.push_back(query);
    }
    std::sort(queries.begin(), queries.end(), tokenComparator);
    tokens.clear();
}

static void removeMultiTokens(TokenArray& tokens, TokenArray& queries)
{
    std::cout<<"Tokens::\n";
    for (std::size_t i = 0; i < tokens.size(); i++)
    {
        if (tokens[i].isCenterToken())
            std::cout<<i<<": "<<tokens[i].token()<<" "<<tokens[i].weight()<<" "<<tokens[i].location()<<"\n";
    }
    //First step: center word
    //TokenArray centerTokens;
    //centerWords(tokens, centerTokens);
    tagCenterTokens(tokens);
    std::cout<<"Center Tokens::\n";
    for (std::size_t i = 0; i < tokens.size(); i++)
    {
        if (tokens[i].isCenterToken())
            std::cout<<i<<": "<<tokens[i].token()<<" "<<tokens[i].weight()<<" "<<tokens[i].location()<<"\n";
    }
    //Second step: expand center word
    expandCenterTokens(tokens);
    std::cout<<"Expaned::\n";
    for(std::size_t i = 0; i < tokens.size(); i++)
    {
        std::cout<<tokens[i].token()<<" "<<tokens[i].weight()<<" "<<tokens[i].location()<<"\n";
    }

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
        queries.reserve(2);
        queries.push_back(tokens[0]);
        queries.push_back(tokens[1]);
        return;
    }
    if ( 3 == tokens.size())
    {
        std::sort(tokens.begin(), tokens.end(), tokenComparator);
        queries.reserve(2);
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
