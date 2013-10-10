#ifndef RECOMMEND_ENGINE_H
#define RECOMMEND_ENGINE_H

#include "tokenize/Tokenizer.h"
#include "parser/Parser.h"
#include "parser/ParserFactory.h"
#include "TermCateTable.h"
#include "UserQueryCateTable.h"
#include "IndexEngine.h"
#include <string>

namespace sf1r
{
namespace Recommend
{
class RecommendEngine
{
public:
    RecommendEngine(const std::string& dir = "");
    ~RecommendEngine();

public:
    void buildEngine(const std::string& path = "");
    void recommend(const std::string& userQuery, const uint32_t N, std::vector<std::string>& results) const;
    void evaluate(const std::string& path = "") const;


    void setTokenizer(Tokenize::Tokenizer* tokenizer)
    {
        tokenizer_ = tokenizer;
    }

    const Tokenize::Tokenizer* tokenizer() const
    {
        return tokenizer_;
    }

    void setWorkDirectory(const std::string& dir)
    {
        workdir_ = dir;
    }

    const std::string& workDirectory() const
    {
        return workdir_;
    }

private:
    void processQuery(const std::string& userQuery, const std::string& category, const uint32_t freq);
private:
    Tokenize::Tokenizer* tokenizer_;
    ParserFactory* parsers_;
    TermCateTable* tcTable_;
    UserQueryCateTable* uqcTable_;
    IndexEngine* indexer_;
    std::string workdir_;
};
}
}

#endif
