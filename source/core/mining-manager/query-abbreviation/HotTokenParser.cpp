#include "HotTokenParser.h"
#include <string.h>
#include <stdlib.h>

namespace sf1r
{
namespace QA
{

uint32_t HotTokenParser::MAX_LINE = 512;
bool HotTokenParser::isValid(const std::string& path)
{
    return true;
}

void HotTokenParser::load(const std::string& path)
{
    if (ifs_.is_open())
        ifs_.close();
    if (isValid(path))
    {
        ifs_.open(path.c_str(), std::ifstream::in);
    }
}
    
bool HotTokenParser::next()
{
    memset(cLine_, 0, MAX_LINE);
    return ifs_.getline(cLine_, MAX_LINE);
}

const HotToken& HotTokenParser::get() const
{
    std::string sLine(cLine_);
    std::size_t pos = sLine.find('\t');
    if (std::string::npos == pos)
    {
        token_->token(sLine);
        token_->factor(0);
    }
    else
    {
        token_->token(sLine.substr(0, pos));
        token_->factor(atoi(sLine.substr(pos + 1).c_str()));
    }
    return *token_;
}

}
}
