#include "TrieProductTokenizer.h"
#include "TrieFactory.h"
#include <boost/filesystem.hpp>
#include <glog/logging.h>

using namespace sf1r;
using izenelib::util::UString;
namespace bfs = boost::filesystem;

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
    if (getBigramTokens_(input, left_tokens))
    {
        for (std::list<std::pair<UString,double> >::iterator it = left_tokens.begin();
                it != left_tokens.end(); ++it)
        {
            if (isProductType_(it->first))
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
