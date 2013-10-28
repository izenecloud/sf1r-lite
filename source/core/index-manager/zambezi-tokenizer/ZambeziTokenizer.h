#ifndef SF1R_COMMON_ZAMBEZI_TOKENIZER_H_
#define SF1R_COMMON_ZAMBEZI_TOKENIZER_H_

#include <util/singleton.h>
#include <am/succinct/ux-trie/uxTrie.hpp>
#include <common/type_defs.h>
#include <string>
#include <list>
#include <map>
#include <vector>

namespace cma
{
class Analyzer;
class Knowledge;
}

namespace sf1r
{

class ZambeziTokenizer
{
public:
    enum TokenizerType
    {
        CMA_MAXPRE, ///type 100 of icma;
        CMA_FMINCOVER ///type 5 of icma
    };

    ZambeziTokenizer(
            TokenizerType type,
            const std::string& dict_path);

    ~ZambeziTokenizer();

    bool GetTokenResults(
            const std::string& pattern,
            std::list<std::pair<std::string, double> >& token_results);
    
private:
    void InitWithCMA_(const std::string& dict_path);

    bool GetTokenResultsByCMA_(
            const std::string& pattern,
            std::list<std::pair<std::string, double> >& token_results);

private:
    TokenizerType type_;

    cma::Analyzer* analyzer_;
    cma::Knowledge* knowledge_;
};

}

#endif
