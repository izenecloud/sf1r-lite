#include "ProductTokenizer.h"
#include <common/CMAKnowledgeFactory.h>

#include <boost/filesystem.hpp>
#include <glog/logging.h>
#include <iostream>

using namespace cma;

namespace sf1r
{
const UString ProductTokenizer::SPACE_UCHAR(" ", UString::UTF_8);

ProductTokenizer::ProductTokenizer(
    TokenizerType type, 
    const std::string& dict_path)
    : type_(type)
    , analyzer_(NULL)
    , knowledge_(NULL)
{
    Init_(dict_path);
}

ProductTokenizer::~ProductTokenizer()
{
    if(analyzer_) delete analyzer_;
}

void ProductTokenizer::Init_(const std::string& dict_path)
{
    switch(type_)
    {
    case TOKENIZER_CMA:
        InitWithCMA_(dict_path);
        break;
    case TOKENIZER_DICT:
        InitWithDict_(dict_path);
        break;
    }
}

void ProductTokenizer::InitWithCMA_(const std::string& dict_path)
{
    knowledge_ = CMAKnowledgeFactory::Get()->GetKnowledge(dict_path, false);
    analyzer_ = CMA_Factory::instance()->createAnalyzer();
    analyzer_->setOption(Analyzer::OPTION_TYPE_POS_TAGGING, 0);
    // using the maxprefix analyzer
    analyzer_->setOption(Analyzer::OPTION_ANALYSIS_TYPE, 100);
    analyzer_->setKnowledge(knowledge_);
    LOG(INFO) << "load dictionary knowledge finished." << endl;
}

void ProductTokenizer::InitWithDict_(const std::string& dict_path)
{
    dict_path_ = dict_path;
    dict_names_.push_back(std::make_pair("category.txt", 5.0));
    dict_names_.push_back(std::make_pair("brand.txt", 4.0));
    dict_names_.push_back(std::make_pair("type.txt", 3.0));
    dict_names_.push_back(std::make_pair("attribute.txt", 2.0));	
    for(std::vector<std::pair<string,double> >::iterator it = dict_names_.begin();
        it != dict_names_.end(); ++it)
        InitDict_(it->first);
}


void ProductTokenizer::InitDict_(const std::string& dict_name)
{
    boost::filesystem::path dic_path(dict_path_);
    dic_path /= boost::filesystem::path(dict_name);
    LOG(INFO) << "Initialize dictionary path : " << dic_path.c_str() << endl;
    izenelib::am::succinct::ux::Trie* trie = TrieFactory::Get()->GetTrie(dic_path.c_str());
    tries_.push_back(trie);
}

void ProductTokenizer::GetDictTokens_(
    const std::list<std::string>& input,
    std::list<std::pair<UString,double> >& tokens,
    std::list<std::string>& left,
    izenelib::am::succinct::ux::Trie* dict_trie,
    double trie_score)
{
    izenelib::am::succinct::ux::id_t retID;
    size_t len, lastpos, beginpos, retLen;
    for(std::list<std::string>::const_iterator it = input.begin(); it != input.end(); ++it)
    {
        const char* start = (*it).c_str();
        len = (*it).length();
        lastpos = beginpos = 0;
        while(beginpos < len)
        {
            retID = dict_trie->prefixSearch(start + beginpos, len - beginpos, retLen);
            if(retID != izenelib::am::succinct::ux::NOTFOUND)
            {
                std::string match = dict_trie->decodeKey(retID);
                tokens.push_back(std::make_pair(UString(match, UString::UTF_8), trie_score));

                if(beginpos > lastpos)
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
       if(beginpos > lastpos)
           left.push_back(std::string(start + lastpos, beginpos - lastpos));
   }
}

inline bool IsChinese(uint16_t val)
{
    return UString::isThisChineseChar(val);
}

inline bool IsNonChinese(uint16_t val)
{
    static const uint16_t dash('-'), underline('_');
    return UString::isThisAlphaChar(val) ||UString::isThisNumericChar(val) ||val == dash || val == underline ;
}

inline bool IsValid(uint16_t val)
{
    return IsNonChinese(val) ||IsChinese(val) ;
}

void ProductTokenizer::GetLeftTokens_(
    const std::list<std::string>& input,
    std::list<std::pair<UString,double> >& tokens)
{
    ///Chinese bigram
    size_t len, i;
    for(std::list<std::string>::const_iterator it = input.begin(); it != input.end(); ++it)
    {
        UString pattern(*it, UString::UTF_8);
        len = pattern.length();
        if(len >= 3)
        {
            std::vector<std::pair<CharType, size_t> > pos; 
            CharType last_type = CHAR_INVALID;
            i = 0;
            while(i < len)
            {
                if(IsChinese(pattern[i]))
                {
                    if(last_type == CHAR_INVALID)
                    {
                        last_type = CHAR_CHINESE;
                        pos.push_back(std::make_pair(CHAR_CHINESE, i));
                    }
                    else
                    {
                        if(pos.back().first != CHAR_CHINESE )
                        {
                            pos.push_back(std::make_pair(CHAR_ALNUM, i));
                            pos.push_back(std::make_pair(CHAR_CHINESE, i));
                            last_type = CHAR_CHINESE;
                        }
                    }
                    ++i;
                }
                else if(IsNonChinese(pattern[i]))
                {
                    if(last_type == CHAR_INVALID)
                    {
                        last_type = CHAR_ALNUM;                    
                        pos.push_back(std::make_pair(CHAR_ALNUM, i));
                    }
                    else
                    {
                        if(pos.back().first !=CHAR_ALNUM )
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
                    if(pos.size() % 2 != 0)
                    {
                        pos.push_back(std::make_pair(pos.back().first, i));
                        last_type = CHAR_INVALID;
                    }
                    do
                    {
                        ++i;
                    }
                    while(!IsValid(pattern[i]) && i < len);
                }
            }
            if(pos.size() % 2 != 0)
            {
                pos.push_back(std::make_pair(pos.back().first, len));
            }

            size_t start, end;
            for(size_t i = 0; i < pos.size(); i += 2)
            {
                start = pos[i].second;
                end = pos[i + 1].second;
                if(pos[i].first == CHAR_CHINESE)
                {
                    if(end - start > 1)
                    {
                        for(size_t i = start; i < end - 1; ++i)
                        {
                            tokens.push_back(std::make_pair(pattern.substr(i, 2), (double)1.0));
                        }
                    }
                    else
                        tokens.push_back(std::make_pair(pattern.substr(start, 1), (double)1.0));
                }
                else
                {
                    tokens.push_back(std::make_pair(pattern.substr(start, end - start), (double)1.0));
                }
            }
        }
        else tokens.push_back(std::make_pair(pattern, (double)1.0));
    }
}

bool ProductTokenizer::GetTokenResults(
    const std::string& pattern, 
    std::list<std::pair<UString,double> >& token_results,
    UString& refined_results)
{
    return type_ == TOKENIZER_DICT? 
        GetTokenResultsByDict_(pattern, token_results, refined_results) : 
        GetTokenResultsByCMA_(pattern, token_results, refined_results);
}

bool ProductTokenizer::GetTokenResultsByCMA_(
    const std::string& pattern, 
    std::list<std::pair<UString,double> >& token_results,
    UString& refined_results)
{
    Sentence pattern_sentence(pattern.c_str());
    analyzer_->runWithSentence(pattern_sentence);
    LOG(INFO) << "query tokenize by maxprefix match in dictionary: ";
    for (int i = 0; i < pattern_sentence.getCount(0); i++)
    {
        printf("%s, ", pattern_sentence.getLexicon(0, i));
        token_results.push_back(std::make_pair(UString(pattern_sentence.getLexicon(0, i), UString::UTF_8), (double)2.0));
    }
    cout << endl;

    if(!token_results.empty())
    {
        for(std::list<std::pair<UString,double> >::iterator it = token_results.begin(); it != token_results.end(); ++it)
        {
            refined_results += it->first;
            refined_results += SPACE_UCHAR;
        }
    }

    LOG(INFO) << "query tokenize by maxprefix match in bigram: ";
    for (int i = 0; i < pattern_sentence.getCount(1); i++)
    {
        printf("%s, ", pattern_sentence.getLexicon(1, i));
        token_results.push_back(std::make_pair(UString(pattern_sentence.getLexicon(1, i), UString::UTF_8), (double)1.0));
    }
    cout << endl;

    return true;
}

bool ProductTokenizer::GetTokenResultsByDict_(
    const std::string& pattern, 
    std::list<std::pair<UString,double> >& token_results,
    UString& refined_results)
{
    std::list<std::string> input, left;
    std::list<UString> hits;
    input.push_back(pattern);

    for(size_t i = 0; i < tries_.size(); ++i)
    {
        GetDictTokens_(input, token_results, left, tries_[i], dict_names_[i].second);
        input.swap(left);
        left.clear();
    }

    if(!token_results.empty())
    {
        for(std::list<std::pair<UString,double> >::iterator it = token_results.begin(); it != token_results.end(); ++it)
        {
            refined_results += it->first;
            refined_results += SPACE_UCHAR;
        }
    }
    GetLeftTokens_(input, token_results);
    return !token_results.empty();
}

}

