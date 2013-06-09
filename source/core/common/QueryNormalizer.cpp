#include "QueryNormalizer.h"
#include <util/ustring/UString.h>
#include <vector>
#include <cctype> // isspace, tolower
#include <algorithm> // sort

using namespace sf1r;
using izenelib::util::UString;

namespace
{
typedef UString::const_iterator UStrIter;
const UString::EncodingType kEncodeType = UString::UTF_8;

/*
 * To make such strings "5.5", "NO.1" as one character,
 * the full stop character is seemed as alphabet type.
 */
const UString::CharT kFullStop = UString(".", kEncodeType)[0];

/*
 * In UString::isThisSpaceChar(), it doesn't recognize the carriage return
 * character "\r" as space type, so we have to check it by ourselves.
 */
const UString::CharT kCarriageReturn = UString("\r", kEncodeType)[0];

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

std::size_t QueryNormalizer::countCharNum(const std::string& query)
{
    std::size_t count = 0;
    bool isAlphaNumChar = false;

    UString ustr(query, kEncodeType);
    const UStrIter endIt = ustr.end();

    for (UStrIter it = ustr.begin(); it != endIt; ++it)
    {
        if (UString::isThisAlphaChar(*it) ||
            UString::isThisNumericChar(*it) ||
            *it == kFullStop)
        {
            if (isAlphaNumChar == false)
            {
                ++count;
                isAlphaNumChar = true;
            }
            continue;
        }

        if (isAlphaNumChar)
        {
            isAlphaNumChar = false;
        }

        if (UString::isThisPunctuationChar(*it) ||
            UString::isThisSpaceChar(*it) ||
            *it == kCarriageReturn)
        {
            continue;
        }

        ++count;
    }

    return count;
}
