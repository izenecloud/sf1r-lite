#include "ProductTokenizer.h"
#include <common/CMAKnowledgeFactory.h>
#include <b5m-manager/product_matcher.h>
#include <la-manager/KNlpWrapper.h>
#include <mining-manager/category-classify/CategoryClassifyTable.h>
#include <common/QueryNormalizer.h>
#include <boost/filesystem.hpp>
#include <glog/logging.h>
#include <iostream>

using namespace cma;

namespace sf1r
{
const UString::CharT ProductTokenizer::SPACE_UCHAR = ' ';

ProductTokenizer::ProductTokenizer(
    TokenizerType type,
    const std::string& dict_path)
    : type_(type)
    , analyzer_(NULL)
    , knowledge_(NULL)
    , matcher_(NULL)
    , categoryClassifyTable_(NULL)
{
    Init_(dict_path);
}

ProductTokenizer::~ProductTokenizer()
{
    if (analyzer_) delete analyzer_;
}

void ProductTokenizer::setCategoryClassifyTable(CategoryClassifyTable* table)
{
    categoryClassifyTable_ = table;
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
    LOG(INFO) << "load dictionary knowledge finished.";
}

void ProductTokenizer::InitWithDict_(const std::string& dict_path)
{
    dict_path_ = dict_path;
    dict_names_.push_back(std::make_pair("category.txt", 5.0));
    dict_names_.push_back(std::make_pair("brand.txt", 4.0));
    dict_names_.push_back(std::make_pair("type.txt", 3.0));
    dict_names_.push_back(std::make_pair("attribute.txt", 2.0));
    for (std::vector<std::pair<string,double> >::iterator it = dict_names_.begin();
            it != dict_names_.end(); ++it)
        InitDict_(it->first);
}


void ProductTokenizer::InitDict_(const std::string& dict_name)
{
    boost::filesystem::path dic_path(dict_path_);
    dic_path /= boost::filesystem::path(dict_name);
    LOG(INFO) << "Initialize dictionary path : " << dic_path.c_str();
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

inline bool IsChinese(uint16_t val)
{
    return UString::isThisChineseChar(val);
}

inline bool IsNonChinese(uint16_t val)
{
    static const uint16_t dash('-'), underline('_'), dot('.');
    return UString::isThisAlphaChar(val)
        || UString::isThisNumericChar(val)
        || val == dash
        || val == underline
        || val == dot;
}

inline bool IsValid(uint16_t val)
{
    return IsNonChinese(val) ||IsChinese(val) ;
}

inline bool isProductType(const UString& str)
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

bool ProductTokenizer::GetSynonymSet(const UString& pattern, std::vector<UString>& synonym_set, int& setid)
{
    if (!matcher_)
    {
        LOG(INFO)<<"matcher_ = NULL";        
        return false;
    }

    return matcher_->GetSynonymSet(pattern, synonym_set, setid);
}

void ProductTokenizer::DoBigram_(
        const UString& pattern,
        std::list<std::pair<UString, double> >& tokens,
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

bool ProductTokenizer::GetLeftTokens_(
        const std::list<std::string>& input,
        std::list<std::pair<UString,double> >& tokens,
        double score)
{
    ///Chinese bigram
    for (std::list<std::string>::const_iterator it = input.begin(); it != input.end(); ++it)
    {
        DoBigram_(UString(*it, UString::UTF_8), tokens, score);
    }

    return !tokens.empty();
}

bool ProductTokenizer::GetLeftTokens_(
        const std::list<UString>& input,
        std::list<std::pair<UString,double> >& tokens,
        double score)
{
    ///Chinese bigram
    for (std::list<UString>::const_iterator it = input.begin(); it != input.end(); ++it)
    {
        DoBigram_(*it, tokens, score);
    }

    return !tokens.empty();
}

bool ProductTokenizer::GetTokenResults(
        const std::string& pattern,
        std::list<std::pair<UString,double> >& major_tokens,
        std::list<std::pair<UString,double> >& minor_tokens,
        UString& refined_results)
{
    std::list<std::pair<UString,double> > temp_major_tokens;
    std::list<std::pair<UString,double> > temp_minor_tokens;
    UString temp_refined_results;

    GetTokenResultsByKNlp_(pattern, minor_tokens, temp_refined_results);

    if (matcher_ && GetTokenResultsByMatcher_(pattern,
                                              temp_major_tokens,
                                              temp_minor_tokens,
                                              refined_results))
    {
        return true;
    }

    switch (type_)
    {
    case TOKENIZER_DICT:
        return GetTokenResultsByDict_(pattern,
                                      temp_minor_tokens,
                                      refined_results);
    case TOKENIZER_CMA:
        return GetTokenResultsByCMA_(pattern,
                                     temp_minor_tokens,
                                     refined_results);
    }

    return false;
}
void ProductTokenizer::GetQuerySumScore(const std::string& pattern, double &sum_score, docid_t docid)
{
    std::list<std::pair<UString,double> > token_results;
    UString refined_results;
    sum_score = GetTokenResultsByKNlp_(pattern, token_results, refined_results, docid);
    //cout << "pattern: " << pattern << " docid :" << docid << " sum_score:" << sum_score << endl;
}

double ProductTokenizer::GetTokenResultsByKNlp_(
        const std::string& pattern,
        std::list<std::pair<UString,double> >& token_results,
        UString& refined_results,
        docid_t docid)
{
    std::string newPattern;
    std::string minor_pattern;
    std::vector<std::string> product_model;
    sf1r::QueryNormalizer::get()->getProductTypes(pattern, product_model, minor_pattern);

    KNlpWrapper::token_score_list_t tokenScores;
    KNlpWrapper::token_score_list_t product_modelScores;
    KNlpWrapper::token_score_list_t minor_patternScore;

    KNlpWrapper::string_t kstr(pattern);
    KNlpWrapper::get()->fmmTokenize(kstr, tokenScores);

    ///deal with "书籍杂志" category
    std::string classifyCategory;
     try
    {
        KNlpWrapper* knlpWrapper = KNlpWrapper::get();
        KNlpWrapper::string_t classifyKStr;
        if (docid == 0)
        {
            string pattern_cat = KNlpWrapper::get()->cleanGarbage(pattern);
            KNlpWrapper::token_score_list_t tokenScores_cat;
            KNlpWrapper::string_t kstr_cat(pattern_cat);
            KNlpWrapper::get()->fmmTokenize(kstr_cat, tokenScores_cat);

            KNlpWrapper::string_t classifyKStr = knlpWrapper->classifyToBestCategory(tokenScores_cat);
            classifyCategory = classifyKStr.get_bytes("utf-8");
        }
        else
        {
            if (categoryClassifyTable_ != NULL)
            {
                classifyCategory = categoryClassifyTable_->getCategoryNoLock(docid).first;
                //cout << "classifyCategory: " << classifyCategory <<endl; 
            }
        }
    }
    catch(std::exception& ex)
    {
        LOG(ERROR) << "exception: " << ex.what();
    }
    
    if (classifyCategory == "R>文娱>书籍杂志" || classifyCategory == "R>文娱>音像影视")
    {
        std::map<UString, double> tokenScoreMap_cat;
        std::list<std::pair<UString, double> > token_results_book;
        std::list<std::pair<UString, double> > token_results_book_minor;
        if (!minor_pattern.empty())
            GetTokenResultsByCMA_(minor_pattern, token_results_book_minor, refined_results);
        GetTokenResultsByCMA_(pattern, token_results_book, refined_results);
        double sum_score = 0;
        for (std::list<std::pair<UString, double> >::iterator i = token_results_book_minor.begin(); i != token_results_book_minor.end(); ++i)
        {
            i->second = (i->second)/10;
            if(tokenScoreMap_cat.insert(*i).second)
                sum_score += i->second;
        }
        for (std::list<std::pair<UString, double> >::iterator i = token_results_book.begin(); i != token_results_book.end(); ++i)
        {
            if (tokenScoreMap_cat.insert(*i).second)
                sum_score += i->second;
        }
        for (std::map<UString, double>::iterator i = tokenScoreMap_cat.begin(); i != tokenScoreMap_cat.end(); ++i)
        {
            token_results.push_back(std::make_pair(i->first, i->second/sum_score));  
        }
        return sum_score;
    }
    ///deal with "书籍杂志" category end;

    if (!minor_pattern.empty())
    {
        KNlpWrapper::string_t kmstr(minor_pattern);
        KNlpWrapper::get()->fmmTokenize(kmstr, minor_patternScore);
    }

    std::vector<KNlpWrapper::string_t> t_product_model;
    for (std::vector<std::string>::iterator i = product_model.begin(); i != product_model.end(); ++i)
    {
        KNlpWrapper::string_t kpmstr(*i);
        t_product_model.push_back(kpmstr);
    }

    double scoreSum = 0;
    KNlpWrapper::category_score_map_t tokenScoreMap;
    for (KNlpWrapper::token_score_list_t::const_iterator it =
             tokenScores.begin(); it != tokenScores.end(); ++it)
        scoreSum += it->second;

    std::ostringstream oss;
    if (product_model.size() == 1)
    {
        product_modelScores.push_back(std::make_pair(t_product_model[0], scoreSum/5));
    }
    else if (product_model.size() == 2)
    {
        for (std::vector<KNlpWrapper::string_t>::iterator i = t_product_model.begin(); i != t_product_model.end(); ++i)
            product_modelScores.push_back(std::make_pair(*i, scoreSum/10));
    }
    else if (product_model.size() == 3)
    {
        for (std::vector<KNlpWrapper::string_t>::iterator i = t_product_model.begin(); i != t_product_model.end(); ++i)
            product_modelScores.push_back(std::make_pair(*i, scoreSum/21));
    }

    scoreSum = 0;

    /// add all term and its score;
    for (KNlpWrapper::token_score_list_t::iterator it =
             minor_patternScore.begin(); it != minor_patternScore.end(); ++it)
    {
        it->second /= 100;
        tokenScoreMap.insert(*it);
        scoreSum += it->second;
    }

    for (KNlpWrapper::token_score_list_t::const_iterator it =
             product_modelScores.begin(); it != product_modelScores.end(); ++it)
    {
        if(tokenScoreMap.insert(*it).second)
            scoreSum += it->second;
    }

    for (KNlpWrapper::token_score_list_t::const_iterator it =
             tokenScores.begin(); it != tokenScores.end(); ++it)
    {
        if(tokenScoreMap.insert(*it).second)
            scoreSum += it->second;
    }

    //maxScore = 0;
    for (KNlpWrapper::category_score_map_t::const_iterator it =
             tokenScoreMap.begin(); it != tokenScoreMap.end(); ++it)
    {
        std::string str = it->first.get_bytes("utf-8");
        UString ustr(str, UString::UTF_8);
        double score = it->second / scoreSum;
        token_results.push_back(std::make_pair(ustr, score));
        oss << str << " ";
    }

    refined_results.assign(oss.str(), UString::UTF_8);
    return scoreSum;
}

bool ProductTokenizer::GetTokenResultsByCMA_(
        const std::string& pattern,
        std::list<std::pair<UString,double> >& token_results,
        UString& refined_results)
{
    Sentence pattern_sentence(pattern.c_str());

    analyzer_->runWithSentence(pattern_sentence);
    //LOG(INFO) << "query tokenize by maxprefix match in dictionary: ";
    for (int i = 0; i < pattern_sentence.getCount(0); ++i)
    {
        //printf("%s, ", pattern_sentence.getLexicon(0, i));
        token_results.push_back(std::make_pair(UString(pattern_sentence.getLexicon(0, i), UString::UTF_8), (double)2.0));
    }
    //cout << endl;

    if (!token_results.empty())
    {
        for (std::list<std::pair<UString,double> >::iterator it = token_results.begin(); it != token_results.end(); ++it)
        {
            refined_results.append(it->first);
            refined_results.push_back(SPACE_UCHAR);
        }
    }

    //LOG(INFO) << "query tokenize by maxprefix match in bigram: ";
    for (int i = 0; i < pattern_sentence.getCount(1); i++)
    {
        //printf("%s, ", pattern_sentence.getLexicon(1, i));
        token_results.push_back(std::make_pair(UString(pattern_sentence.getLexicon(1, i), UString::UTF_8), (double)1.0));
    }
    //cout << endl;

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

    for (size_t i = 0; i < tries_.size(); ++i)
    {
        GetDictTokens_(input, token_results, left, tries_[i], dict_names_[i].second);
        input.swap(left);
        left.clear();
    }

    if (!token_results.empty())
    {
        for (std::list<std::pair<UString,double> >::iterator it = token_results.begin(); it != token_results.end(); ++it)
        {
            refined_results.append(it->first);
            refined_results.push_back(SPACE_UCHAR);
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
                refined_results.append(it->first);
                refined_results.push_back(SPACE_UCHAR);
            }
        }
        token_results.splice(token_results.end(), left_tokens);
    }

    return !token_results.empty();
}

bool ProductTokenizer::GetTokenResultsByMatcher_(
        const std::string& pattern,
        std::list<std::pair<UString, double> >& major_tokens,
        std::list<std::pair<UString, double> >& minor_tokens,
        UString& refined_results)
{
    if (!matcher_) return false;

    UString patternUStr(pattern, UString::UTF_8);
    std::list<UString> left;

    matcher_->GetSearchKeywords(patternUStr, major_tokens, minor_tokens, left);

    if (!major_tokens.empty())
    {
        std::list<std::pair<UString,double> >::iterator it = major_tokens.begin();
        while (it != major_tokens.end())
        {
            if (patternUStr.find(it->first) != UString::npos)
            {
                refined_results.append(it->first);
                refined_results.push_back(SPACE_UCHAR);
                ++it;
            }
            else
            {
                it = major_tokens.erase(it);
            }
        }
    }

    std::list<std::pair<UString, double> > left_tokens;
    if (GetLeftTokens_(left, left_tokens, 0.1))
    {
        minor_tokens.splice(minor_tokens.end(), left_tokens);
    }

    return !(major_tokens.empty() && minor_tokens.empty());
}

}
