/**
 * @file ProductTokenizer.h
 * @brief tokenize product title and query
 */

#ifndef PRODUCT_TOKENIZER_H
#define PRODUCT_TOKENIZER_H

#include <util/ustring/UString.h>
#include <string>
#include <list>

namespace sf1r
{

struct ProductTokenParam
{
    const std::string& query;
    const bool isRefineResult;

    typedef std::pair<izenelib::util::UString, double> TokenScore;
    typedef std::list<TokenScore> TokenScoreList;
    typedef TokenScoreList::const_iterator TokenScoreListIter;

    TokenScoreList majorTokens;
    TokenScoreList minorTokens;

    izenelib::util::UString refinedResult;

    double rankBoundary;

    ProductTokenParam(const std::string& queryParam, bool isRefineParam)
            : query(queryParam)
            , isRefineResult(isRefineParam)
            , rankBoundary(0)
    {}
};

class ProductTokenizer
{
public:
    virtual ~ProductTokenizer() {}

    virtual void tokenize(ProductTokenParam& param) = 0;

    virtual double sumQueryScore(const std::string& query)
    {
        return 0;
    }

    virtual double sumQueryScore(
        const std::string& query,
        std::string& classifyCategory)
    {
        return 0;
    }

protected:
    static const izenelib::util::UString::CharT SPACE_UCHAR = ' ';
};

}

#endif // PRODUCT_TOKENIZER_H
