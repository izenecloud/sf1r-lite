#include "TrieProductTokenizer.h"
#include "TrieFactory.h"
#include <boost/filesystem.hpp>
#include <glog/logging.h>

using namespace sf1r;
using izenelib::util::UString;
namespace bfs = boost::filesystem;

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

bool isProductType(const UString& str)
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

}

TrieProductTokenizer::TrieProductTokenizer(const std::string& dictDir)
{
    dict_names_.push_back(std::make_pair("category.txt", 5.0));
    dict_names_.push_back(std::make_pair("brand.txt", 4.0));
    dict_names_.push_back(std::make_pair("type.txt", 3.0));
    dict_names_.push_back(std::make_pair("attribute.txt", 2.0));

    for (std::vector<std::pair<string,double> >::iterator it = dict_names_.begin();
         it != dict_names_.end(); ++it)
    {
        bfs::path path(dictDir);
        path /= it->first;
        LOG(INFO) << "Initialize dictionary path : " << path;

        izenelib::am::succinct::ux::Trie* trie =
                TrieFactory::Get()->GetTrie(path.string());

        tries_.push_back(trie);
    }
}

void TrieProductTokenizer::tokenize(ProductTokenParam& param)
{
    ProductTokenParam::TokenScoreList& minorTokens = param.minorTokens;

    std::list<std::string> input, left;
    std::list<UString> hits;
    input.push_back(param.query);

    for (size_t i = 0; i < tries_.size(); ++i)
    {
        GetDictTokens_(input, minorTokens, left, tries_[i], dict_names_[i].second);
        input.swap(left);
        left.clear();
    }

    if (param.isRefineResult)
    {
        for (ProductTokenParam::TokenScoreListIter it = minorTokens.begin();
             it != minorTokens.end(); ++it)
        {
            param.refinedResult.append(it->first);
            param.refinedResult.push_back(SPACE_UCHAR);
        }
    }

    std::list<std::pair<UString, double> > left_tokens;
    if (GetLeftTokens_(input, left_tokens))
    {
        for (std::list<std::pair<UString,double> >::iterator it = left_tokens.begin();
                it != left_tokens.end(); ++it)
        {
            if (isProductType(it->first))
            {
                it->second += 1.0;
                if (param.isRefineResult)
                {
                    param.refinedResult.append(it->first);
                    param.refinedResult.push_back(SPACE_UCHAR);
                }
            }
        }
        minorTokens.splice(minorTokens.end(), left_tokens);
    }
}

void TrieProductTokenizer::GetDictTokens_(
    const std::list<std::string>& input,
    std::list<std::pair<izenelib::util::UString,double> >& tokens,
    std::list<std::string>& left,
    izenelib::am::succinct::ux::Trie* dict_trie,
    double trie_score)
{
    izenelib::am::succinct::ux::id_t retID;
    size_t len, lastpos, beginpos, retLen;
    for (std::list<std::string>::const_iterator it = input.begin(); it != input.end(); ++it)
    {
        const char* start = it->c_str();
        len = it->length();
        lastpos = beginpos = 0;
        while (beginpos < len)
        {
            retID = dict_trie->prefixSearch(start + beginpos, len - beginpos, retLen);
            if (retID != izenelib::am::succinct::ux::NOTFOUND)
            {
                std::string match = dict_trie->decodeKey(retID);
                tokens.push_back(std::make_pair(UString(match, UString::UTF_8), trie_score));

                if (beginpos > lastpos)
                {
                    left.push_back(std::string(start + lastpos, beginpos - lastpos));
                }
                beginpos += match.length();
                lastpos = beginpos;
            }
            else
            {
                ++beginpos;
            }
        }
        if (beginpos > lastpos)
        {
            left.push_back(std::string(start + lastpos, beginpos - lastpos));
        }
    }
}

bool TrieProductTokenizer::GetLeftTokens_(
    const std::list<std::string>& input,
    std::list<std::pair<izenelib::util::UString,double> >& tokens,
    double score)
{
    ///Chinese bigram
    for (std::list<std::string>::const_iterator it = input.begin(); it != input.end(); ++it)
    {
        DoBigram_(UString(*it, UString::UTF_8), tokens, score);
    }

    return !tokens.empty();
}

void TrieProductTokenizer::DoBigram_(
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
