#include <mining-manager/suffix-match-manager/ProductTokenizer.h>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include <iostream>
#include <fstream>

#define DIR_PREFIX "./tmp/dict"

using namespace sf1r;
namespace bfs = boost::filesystem;
using izenelib::util::UString;


static const char* Category [] = {
    "中文ab",
    "中文"
};

static const char* Brand [] = {
    "abc",
    "def"
};

static const char* Type [] = {
    "123",
    "456"
};

static const char* Attribute [] = {
    "abc",
    "def"
};

void BuildDict(const std::string& home)
{
    {
    std::ofstream ofs((home + "/category.txt").c_str());
    unsigned sz = sizeof(Category)/sizeof(const char *);
    for(unsigned i = 0; i < sz; ++i)
        ofs << Category[i] << std::endl;
     ofs.close();
    }
    {
    std::ofstream ofs((home + "/brand.txt").c_str());
    unsigned sz = sizeof(Brand)/sizeof(const char *);
    for(unsigned i = 0; i < sz; ++i)
        ofs << Brand[i] << std::endl;
     ofs.close();
    }
    {
    std::ofstream ofs((home + "/type.txt").c_str());
    unsigned sz = sizeof(Type)/sizeof(const char *);
    for(unsigned i = 0; i < sz; ++i)
        ofs << Type[i] << std::endl;
     ofs.close();
    }
    {
    std::ofstream ofs((home + "/attribute.txt").c_str());
    unsigned sz = sizeof(Attribute)/sizeof(const char *);
    for(unsigned i = 0; i < sz; ++i)
        ofs << Attribute[i] << std::endl;
     ofs.close();
    }
}

namespace sf1r{
class ProductTokenizerTest
{
public:
    ProductTokenizerTest(ProductTokenizer& tokenizer)
        :tokenizer_(tokenizer)
    {}

    void tokenize_bigram(const std::string& pattern);

    void tokenize_dict(const std::string& pattern, unsigned idx);

    void tokenize(const std::string& pattern, std::string& refined_results);

    ProductTokenizer& tokenizer_;
};

void ProductTokenizerTest::tokenize_bigram( const std::string& pattern)
{
    typedef std::list<std::pair<UString,double> > tokens_type;

    tokens_type token_results;
    std::list<std::string> input;
    input.push_back(pattern);
    tokenizer_.GetLeftTokens_(input, token_results);
    std::cout<<"tokenization:";
    for(tokens_type::iterator tit = token_results.begin(); tit != token_results.end(); ++tit)
    {
        std::string str;
        tit->first.convertString(str, UString::UTF_8);
        std::cout << str <<" ";
    }
    std::cout<<std::endl;
}

void ProductTokenizerTest::tokenize_dict( const std::string& pattern, unsigned idx)
{
    typedef std::list<std::pair<UString,double> > tokens_type;

    tokens_type token_results;
    std::list<std::string> input, left;
    input.push_back(pattern);
    tokenizer_.GetDictTokens_(input, token_results, left, tokenizer_.tries_[idx], tokenizer_.dict_names_[idx].second);
    std::cout<<"tokenization:";
    for(tokens_type::iterator tit = token_results.begin(); tit != token_results.end(); ++tit)
    {
        std::string str;
        tit->first.convertString(str, UString::UTF_8);
        std::cout << str <<" ";
    }
    std::cout<<std::endl;
    std::cout<<"tokenization left:";
    for(std::list<std::string>::iterator tit = left.begin(); tit != left.end(); ++tit)
    {
        std::cout << *tit <<" ";
    }
    std::cout<<std::endl;
}


void ProductTokenizerTest::tokenize( const std::string& pattern, std::string& refined)
{
    typedef std::list<std::pair<UString,double> > tokens_type;

    tokens_type token_results;
    UString refined_results(refined, UString::UTF_8);
    tokenizer_.GetTokenResults(pattern, token_results, refined_results);
    std::cout<<"tokenization:";
    for(tokens_type::iterator tit = token_results.begin(); tit != token_results.end(); ++tit)
    {
        std::string str;
        tit->first.convertString(str, UString::UTF_8);
        std::cout << str <<" ";
    }
    std::cout<<std::endl;
}

}

BOOST_AUTO_TEST_SUITE(ProductTokenizer_test)
BOOST_AUTO_TEST_CASE(tokenize_bigram)
{
    ProductTokenizer tokenizer(ProductTokenizer::TOKENIZER_DICT,"./");
    ProductTokenizerTest test(tokenizer);
    {
        std::string pattern("a");
        test.tokenize_bigram(pattern);
    }
    {
        std::string pattern("abab");
        test.tokenize_bigram(pattern);
    }
    {
        std::string pattern("中文中文");
        test.tokenize_bigram(pattern);
    }
    {
        std::string pattern("中文ab中123cd中文中文");
        test.tokenize_bigram(pattern);
    }
    {
        std::string pattern("abcd中文中文abcd");
        test.tokenize_bigram(pattern);
    }
    {
        std::string pattern("中文，ab@123cd,defg,中文中文");
        test.tokenize_bigram(pattern);
    }
    {
        std::string pattern("......“中...文,中问...。，12,cd中,,文,ab,”");
        test.tokenize_bigram(pattern);
    }
}

BOOST_AUTO_TEST_CASE(tokenize_dict)
{
    bfs::path dict_dir(DIR_PREFIX);
    boost::filesystem::remove_all(dict_dir);
    bfs::create_directories(dict_dir);
    BuildDict(DIR_PREFIX);

    ProductTokenizer tokenizer(ProductTokenizer::TOKENIZER_DICT,DIR_PREFIX);
    ProductTokenizerTest test(tokenizer);
    {
        std::string pattern("中文ab中123cd中文中文");
        test.tokenize_dict(pattern,0);
    }
    {
        std::string pattern("中文ab中123cd中文中文");
        test.tokenize_dict(pattern,0);
    }
    {
        std::string pattern("ab，中，文ab中123+！cd中文中文abc");
        test.tokenize_dict(pattern,0);
    }
}

BOOST_AUTO_TEST_CASE(tokenize)
{
    bfs::path dict_dir(DIR_PREFIX);
    boost::filesystem::remove_all(dict_dir);
    bfs::create_directories(dict_dir);
    BuildDict(DIR_PREFIX);

    ProductTokenizer tokenizer(ProductTokenizer::TOKENIZER_DICT,DIR_PREFIX);
    ProductTokenizerTest test(tokenizer);
    {
        std::string pattern("中文abc"), refined;
        test.tokenize(pattern, refined);
    }
    {
        std::string pattern("ab，中，文ab中123+！cd中文中文abc"), refined;
        test.tokenize(pattern, refined);
    }
    {
        std::string pattern("MERRTO迈途 新品 户外休闲鞋 M18138-18139 男棕黄 42"), refined;
        test.tokenize(pattern, refined);
    }

}

BOOST_AUTO_TEST_SUITE_END()

