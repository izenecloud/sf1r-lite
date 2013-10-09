#ifndef QUERY_GENERATOR_H
#define QUERY_GENERATOR_H

#include <string>
#include <vector>
#include <list>

#include <boost/function.hpp>
#include <util/ustring/UString.h>

#include "../tokenize/Tokenizer.h"

namespace sf1r
{
namespace Recommend
{

class QueryGenerator
{
public:
    QueryGenerator(const std::string& scdDir);
public:
    void generate();
    
    void setTokenizer(Tokenize::Tokenizer* tokenizer)
    {
        tokenizer_ = tokenizer;
    }

private:
    void open_();
private:
    std::string scdDir_;
    std::vector<std::string> scdFiles_;
    Tokenize::Tokenizer* tokenizer_;
};

}
}
#endif
