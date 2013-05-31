#include "QueryNormalizer.h"
#include <vector>
#include <cctype> // isspace, tolower
#include <algorithm> // sort

using namespace sf1r;

namespace
{
const char kMergeTokenDelimiter = ' ';

void tokenizeQuery(const std::string& query, std::vector<std::string>& tokens)
{
    std::string current;
    const std::size_t length = query.size();

    for (std::size_t i = 0; i < length; ++i)
    {
        const char ch = query[i];

        if (!std::isspace(ch))
        {
            current += std::tolower(ch);
        }
        else if (!current.empty())
        {
            tokens.push_back(current);
            current.clear();
        }
    }

    if (!current.empty())
    {
        tokens.push_back(current);
    }
}

void mergeTokens(const std::vector<std::string>& tokens, std::string& output)
{
    output.clear();
    const int lastPos = static_cast<int>(tokens.size()) - 1;

    for (int i = 0; i < lastPos; ++i)
    {
        output += tokens[i];
        output += kMergeTokenDelimiter;
    }

    if (lastPos >= 0)
    {
        output += tokens[lastPos];
    }
}

} // namespace

void QueryNormalizer::normalize(const std::string& fromStr, std::string& toStr)
{
    std::vector<std::string> tokens;

    tokenizeQuery(fromStr, tokens);

    std::sort(tokens.begin(), tokens.end());

    mergeTokens(tokens, toStr);
}
