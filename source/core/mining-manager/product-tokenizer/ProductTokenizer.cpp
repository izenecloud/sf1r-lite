#include "ProductTokenizer.h"

using namespace sf1r;
using izenelib::util::UString;

namespace
{

enum CharType
{
    CHAR_CHINESE, ///Chinese
    CHAR_ALNUM, ///Alphebet and digitals
    CHAR_INVALID ///Invalid characters
};

bool IsChinese(uint16_t val)
{
    return UString::isThisChineseChar(val);
}

bool IsNonChinese(uint16_t val)
{
    static const uint16_t dash('-'), underline('_'), dot('.');
    return UString::isThisAlphaChar(val)
        || UString::isThisNumericChar(val)
        || val == dash
        || val == underline
        || val == dot;
}

bool IsValid(uint16_t val)
{
    return IsNonChinese(val) || IsChinese(val);
}

}

bool ProductTokenizer::isProductType_(const izenelib::util::UString& str)
{
    size_t length = str.length();
    if (length < 3) return false;
    for (size_t index = 0; index < length; ++index)
    {
        if (!IsNonChinese(str[index]))
        {
            return false;
        }
    }
    std::find_if(str.begin(), str.end(), IsNonChinese);
    return true;
}

bool ProductTokenizer::getBigramTokens_(
    const std::list<std::string>& input,
    std::list<std::pair<izenelib::util::UString, double> >& tokens,
    double score)
{
    for (std::list<std::string>::const_iterator it = input.begin(); it != input.end(); ++it)
    {
        doBigram_(UString(*it, UString::UTF_8), tokens, score);
    }

    return !tokens.empty();
}

bool ProductTokenizer::getBigramTokens_(
    const std::list<izenelib::util::UString>& input,
    std::list<std::pair<izenelib::util::UString, double> >& tokens,
    double score)
{
    for (std::list<UString>::const_iterator it = input.begin(); it != input.end(); ++it)
    {
        doBigram_(*it, tokens, score);
    }

    return !tokens.empty();
}

void ProductTokenizer::doBigram_(
    const izenelib::util::UString& pattern,
    std::list<std::pair<izenelib::util::UString, double> >& tokens,
    double score)
{
    size_t i, len = pattern.length();
    if (len >= 3)
    {
        std::vector<std::pair<CharType, size_t> > pos;
        CharType last_type = CHAR_INVALID;
        i = 0;
        while (i < len)
        {
            if (IsChinese(pattern[i]))
            {
                if (last_type == CHAR_INVALID)
                {
                    last_type = CHAR_CHINESE;
                    pos.push_back(std::make_pair(CHAR_CHINESE, i));
                }
                else
                {
                    if (pos.back().first != CHAR_CHINESE)
                    {
                        pos.push_back(std::make_pair(CHAR_ALNUM, i));
                        pos.push_back(std::make_pair(CHAR_CHINESE, i));
                        last_type = CHAR_CHINESE;
                    }
                }
                ++i;
            }
            else if (IsNonChinese(pattern[i]))
            {
                if (last_type == CHAR_INVALID)
                {
                    last_type = CHAR_ALNUM;
                    pos.push_back(std::make_pair(CHAR_ALNUM, i));
                }
                else
                {
                    if (pos.back().first !=CHAR_ALNUM)
                    {
                        pos.push_back(std::make_pair(CHAR_CHINESE, i));
                        pos.push_back(std::make_pair(CHAR_ALNUM, i));
                        last_type = CHAR_ALNUM;
                    }
                }
                ++i;
            }
            else
            {
                if (pos.size() % 2 != 0)
                {
                    pos.push_back(std::make_pair(pos.back().first, i));
                    last_type = CHAR_INVALID;
                }
                do
                {
                    ++i;
                }
                while (!IsValid(pattern[i]) && i < len);
            }
        }
        if (pos.size() % 2 != 0)
        {
            pos.push_back(std::make_pair(pos.back().first, len));
        }

        size_t start, end;
        for (size_t i = 0; i < pos.size(); i += 2)
        {
            start = pos[i].second;
            end = pos[i + 1].second;
            if (pos[i].first == CHAR_CHINESE)
            {
                if (end - start > 1)
                {
                    for (size_t i = start; i < end - 1; ++i)
                    {
                        tokens.push_back(std::make_pair(pattern.substr(i, 2), score));
                    }
                }
                else
                    tokens.push_back(std::make_pair(pattern.substr(start, 1), score));
            }
            else
            {
                tokens.push_back(std::make_pair(pattern.substr(start, end - start), score));
            }
        }
    }
    else
    {
        tokens.push_back(std::make_pair(pattern, score));
    }
}
