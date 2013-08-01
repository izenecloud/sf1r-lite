/**
 * @file KNlpWrapper.h
 * @brief a wrapper class for ilplib::knlp functions.
 */

#ifndef SF1R_KNLP_WRAPPER_H
#define SF1R_KNLP_WRAPPER_H

#include <knlp/dictionary.h>
#include <util/string/kstring.hpp>
#include <util/singleton.h>
#include <vector>
#include <map>
#include <string>
#include <boost/scoped_ptr.hpp>

namespace ilplib
{
namespace knlp
{
    class Fmm;
    class GarbagePattern;
    class MaxentClassify;
}
}

namespace sf1r
{

class KNlpWrapper
{
public:
    static KNlpWrapper* get();

    KNlpWrapper();

    void setDictDir(const std::string& dictDir);
    bool loadDictFiles();

    typedef izenelib::util::KString string_t;
    typedef std::pair<string_t, double> token_score_pair_t;
    typedef std::vector<token_score_pair_t> token_score_list_t;
    typedef std::map<string_t, double> token_score_map_t;
    typedef std::map<std::string, double> category_score_map_t;

    /**
     * Tokenize @p str into @p tokenScores with forward maximize match.
     */
    void fmmTokenize(const string_t& str, token_score_list_t& tokenScores);

    category_score_map_t classifyToMultiCategories(
        const std::string& str,
        bool isLongStr);

    std::string getBestCategory(const category_score_map_t& categoryScoreMap);

    std::string mapFromOriginalCategory(const std::string& originalCategory);

    std::string cleanGarbage(const std::string& str);

    std::string cleanStopword(const std::string& str);

    void gauss_smooth(std::vector<double>& r);

    void fmmBigram(std::vector<std::pair<KString,double> >& r);

    void fmmBigram_with_space(std::vector<std::pair<KString,double> >& r);

private:
    bool isDictLoaded_;
    std::string dictDir_;

    boost::scoped_ptr<ilplib::knlp::Fmm> tokenizer_;

    boost::scoped_ptr<ilplib::knlp::DigitalDictionary> cateDict_;
    boost::scoped_ptr<ilplib::knlp::VectorDictionary> termMultiCatesDict_;
    boost::scoped_ptr<ilplib::knlp::VectorDictionary> termMultiCatesCondDict_;
    boost::scoped_ptr<ilplib::knlp::Dictionary> originalToClassifyCateDict_;

    boost::scoped_ptr<ilplib::knlp::GarbagePattern> garbagePattern_;

    boost::scoped_ptr<ilplib::knlp::MaxentClassify> longStrClassifier_;

    boost::scoped_ptr<ilplib::knlp::MaxentClassify> shortStrClassifier_;
};

} // namespace sf1r

#endif // SF1R_KNLP_WRAPPER_H
