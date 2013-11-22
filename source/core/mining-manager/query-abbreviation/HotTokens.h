#ifndef SF1R_QA_HOT_TOKENS_H
#define SF1R_QA_HOT_TOKENS_H

#include <boost/unordered_map.hpp>

namespace sf1r
{
namespace QA
{
class HotTokens
{
typedef boost::unordered_map<std::string, uint32_t> TokenContainer;
public:
    HotTokens(const std::string& workdir);

public:
    bool isNeedBuild(const std::string& path);
    void build(const std::string& path);
    const uint32_t search(const std::string& key);
    void save() const;
    void load();
    void clear();

private:
    const std::string workdir_;
    TokenContainer hotTokens_;
};
}
}
#endif
