#ifndef SF1R_QA_ABBR_ENGINE_H
#define SF1R_QA_ABBR_ENGINE_H

#include "HotTokens.h"
#include <mining-manager/query-recommendation/tokenize/Tokenizer.h>

namespace sf1r
{
namespace QA
{
class AbbrEngine
{
public:
    AbbrEngine();
    ~AbbrEngine();

    static AbbrEngine* get();
public:
    void init(const std::string& workdir, const std::string& resdir);
    void abbreviation(const std::string& userQuery, std::vector<std::string>& abbrs);

private:
    HotTokens* hot_;
    std::string workdir_;
    std::string resdir_;
    sf1r::Recommend::Tokenize::Tokenizer* tokenizer_;
    bool isInited_;
};
}
}

#endif
