#ifndef RECOMMEND_ENGINE_H
#define RECOMMEND_ENGINE_H

#include "parser/Parser.h"
#include "parser/ParserFactory.h"
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
    bool isNeedBuild(const std::string& path = "") const;
    void recommend(const std::string& userQuery, const uint32_t N, std::vector<std::string>& results) const;
    void evaluate(const std::string& path = "") const;
    
    void flush() const;
    void clear();

    void setTokenizer(Tokenize::Tokenizer* tokenizer)
    {
        indexer_->setTokenizer(tokenizer);
    }

private:
    void processQuery(const std::string& userQuery, const std::string& category, const uint32_t freq);
    void recommend_(const std::string& userQuery, const uint32_t N, std::vector<std::string>& results) const;
private:
    ParserFactory* parsers_;
    UserQueryCateTable* uqcTable_;
    IndexEngine* indexer_;
    std::time_t timestamp_;
    std::string workdir_;
    
    DISALLOW_COPY_AND_ASSIGN(RecommendEngine);
};
}
}

#endif
