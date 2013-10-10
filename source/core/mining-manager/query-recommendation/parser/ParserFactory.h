#ifndef PARSER_FACTORY_H
#define PARSER_FACTORY_H

#include "TFParser.h"
#include "TaobaoParser.h"

namespace sf1r
{
namespace Recommend
{
class ParserFactory
{
public: 
    static bool isValid(const std::string& path)
    {
        return TaobaoParser::isValid(path) || TFParser::isValid(path);
    }

    Parser* load(const std::string& path)
    {
        if (TaobaoParser::isValid(path))
        {
            Parser* p = new TaobaoParser();
            p->load(path);
            return p;
        }
        if (TFParser::isValid(path))
        {
            Parser* p = new TFParser();
            p->load(path);
            return p;
        }
        return NULL;
    }
};
}
}

#endif
