/**
 * @file KNlpWrapper.h
 * @brief a wrapper class for ilplib::knlp functions.
 */

#ifndef SF1R_KNLP_WRAPPER_H
#define SF1R_KNLP_WRAPPER_H

#include <knlp/tokenize.h>
#include <knlp/dictionary.h>
#include <util/string/kstring.hpp>
#include <util/singleton.h>
#include <vector>
#include <map>
#include <boost/scoped_ptr.hpp>

namespace sf1r
{

class KNlpWrapper
{
public:
    static KNlpWrapper* get()
    {
        return izenelib::util::Singleton<KNlpWrapper>::get();
    }

    bool initTokenizer(const std::string& dictDir);
    bool initClassifier(const std::string& dictDir);

    typedef izenelib::util::KString string_t;
    typedef std::pair<string_t, double> token_score_pair_t;
    typedef std::vector<token_score_pair_t> token_score_list_t;
    typedef std::map<string_t, double> category_score_map_t;

    /**
     * Tokenize @p str into @p tokenScores with forward maximize match.
     */
    void fmmTokenize(const string_t& str, token_score_list_t& tokenScores);

    string_t classifyToBestCategory(const token_score_list_t& tokenScores);

    category_score_map_t classifyToMultiCategories(
        const token_score_list_t& tokenScores);

private:
    boost::scoped_ptr<ilplib::knlp::Tokenize> tokenizer_;

    boost::scoped_ptr<ilplib::knlp::DigitalDictionary> cateDict_;
    boost::scoped_ptr<ilplib::knlp::DigitalDictionary> termCateDict_;
    boost::scoped_ptr<ilplib::knlp::Dictionary> termMultiCatesDict_;
};

} // namespace sf1r

#endif // SF1R_KNLP_WRAPPER_H
