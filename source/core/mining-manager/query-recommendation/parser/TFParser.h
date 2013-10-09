#ifndef TF_PARSER_H
#define TF_PARSER_H

#include "Parser.h"
#include <fstream>

namespace sf1r
{
namespace Recommend
{
class TFParser : public Parser
{
public:
    TFParser()
        : cLine_(NULL)
        , query_(NULL)
    {
        cLine_ = new char[LINE_SIZE];
        query_ = new UserQuery();
    }

    virtual ~TFParser()
    {
        if (in_.is_open())
            in_.close();
        if (NULL != cLine_)
        {
            delete cLine_;
            cLine_ = NULL;
        }
        if (NULL != query_)
        {
            delete query_;
            query_ = NULL;
        }
    }
public:
    virtual void load(const std::string& path);
    virtual bool next();
    virtual const UserQuery& get() const;
    static bool isValid(const std::string& path);
private:
    static void lineToUserQuery(const std::string& str, UserQuery& query);
private:
    std::ifstream in_;
    char* cLine_;
    UserQuery* query_;
    static int LINE_SIZE;
};
}
}

#endif
