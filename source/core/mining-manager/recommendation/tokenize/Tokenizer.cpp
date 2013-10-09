#include "Tokenizer.h"

#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <la-manager/KNlpWrapper.h>
#include <util/ustring/UString.h>

#include <knlp/fmm.h>

namespace sf1r
{
namespace Recommend
{
namespace Tokenize
{
using namespace izenelib::util;
class PTokenizer
{
public:
    PTokenizer(const std::string& dict_path)
    {
        std::string re = dict_path + "/";
        re += "term.txt";
        tokenizer_ = new ilplib::knlp::Fmm(re);
    }
    ~PTokenizer()
    {
        delete tokenizer_;
    }
public:
    void tokenize(const std::string& str, TermVector& tv)
    {
        KNlpWrapper::string_t kstr(str);
        ilplib::knlp::Normalize::normalize(kstr);
        KNlpWrapper::token_score_list_t tokenScores;
        tokenizer_->fmm(kstr, tokenScores);
        tokenScores = tokenizer_->subtokens(tokenScores);
        
        for (std::size_t i = 0; i < tokenScores.size(); i++)
        {
            std::string str = tokenScores[i].first.get_bytes("utf-8");
            UString ustr(str, UString::UTF_8);
            Term t(ustr, tokenScores[i].second);
            tv.push_back(t);
        }
        return;
    }
private:
   ilplib::knlp::Fmm* tokenizer_; 
};

Tokenizer* getTokenizer()
{
    static PTokenizer* pt = new PTokenizer("/home/kevinlin/codebase/sf1r-engine/package/resource/dict/term_category/");
    static Tokenizer tokenizer = boost::bind(&PTokenizer::tokenize, pt, _1, _2);
    return &tokenizer;
}

}
}
}
