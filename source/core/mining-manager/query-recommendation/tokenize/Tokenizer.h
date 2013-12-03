#ifndef SF1R_RECOMMEND_TOKENIZE_TOKENIZER_H
#define SF1R_RECOMMEND_TOKENIZE_TOKENIZER_H

#include <string>
#include <util/ustring/UString.h>
#include <vector>

namespace sf1r
{
namespace Recommend
{
namespace Tokenize
{
class Term
{
public:
    Term(izenelib::util::UString str, double freq)
    {
        pair_= std::make_pair(str, freq);
    }
    
    std::string term()
    {
        std::string sterm;
        pair_.first.convertString(sterm, izenelib::util::UString::UTF_8);
        return sterm;
    }

    double freq()
    {
        return pair_.second;
    }
private:
    std::pair<izenelib::util::UString, double> pair_;
};

typedef std::vector<Term> TermVector;
typedef boost::function<void(const std::string&, TermVector&)> Tokenizer;

Tokenizer* getTokenizer_();
}
}
}
#endif
