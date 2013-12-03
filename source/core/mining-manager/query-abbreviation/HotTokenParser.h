#ifndef SF1R_QA_HOT_TOKEN_PARSER_H
#define SF1R_QA_HOT_TOKEN_PARSER_H

#include <string>
#include <stdint.h>
#include <fstream>

namespace sf1r
{
namespace QA
{
class HotToken
{
public:
    void token(const std::string& token)
    {
        token_ = token;
    }

    const std::string& token() const
    {
        return token_;
    }

    void factor(const uint32_t factor)
    {
        factor_ = factor;
    }

    uint32_t factor() const
    {
        return factor_;
    }

private:
    std::string token_;
    uint32_t factor_;
};

class HotTokenParser
{
public:
    HotTokenParser()
        : token_(NULL)
        , cLine_(NULL)
    {
        token_ = new HotToken();
        cLine_ = new char[MAX_LINE];
    }

    virtual ~HotTokenParser()
    {
        if (NULL != token_)
        {
            delete token_;
            token_ = NULL;
        }
        if (NULL != cLine_)
        {
            delete[] cLine_;
            cLine_ = NULL;
        }
    }
public:
    void load(const std::string& path);
    bool next();
    const HotToken& get() const;

    static bool isValid(const std::string& path);

private:
    HotToken* token_;
    char* cLine_;
    std::ifstream ifs_;
    static uint32_t MAX_LINE;
};
}
}

#endif
