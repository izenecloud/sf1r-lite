/**
 * @file FuzzySearchRanker.h
 * @brief rank the fuzzy search results.
 * @author Vincent Lee, Jun Jiang
 * @date Created 2013-01-11
 */

#ifndef SF1R_FUZZY_SEARCH_RANKER_H
#define SF1R_FUZZY_SEARCH_RANKER_H

#include <common/inttypes.h>
#include <vector>

namespace sf1r
{
class SearchManagerPreProcessor;
class ProductRankingConfig;
class SearchKeywordOperation;

class FuzzySearchRanker
{
public:
    FuzzySearchRanker(SearchManagerPreProcessor& preprocessor);

    void setFuzzyScoreWeight(const ProductRankingConfig& rankConfig);

    void rank(
        const SearchKeywordOperation& actionOperation,
        uint32_t start,
        std::vector<uint32_t>& docid_list,
        std::vector<float>& result_score_list,
        std::vector<float>& custom_score_list);

private:
    SearchManagerPreProcessor& preprocessor_;

    score_t fuzzyScoreWeight_;
};

} // namespace sf1r

#endif // SF1R_FUZZY_SEARCH_RANKER_H
