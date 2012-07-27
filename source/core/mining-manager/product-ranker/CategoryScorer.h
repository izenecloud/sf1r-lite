/**
 * @file CategoryScorer.h
 * @brief it gives the category score, which is calculated by below priority:
 * 1. label click count
 * 2. average value from CategoryScorer::avgPropNames_
 * @author Jun Jiang <jun.jiang@izenesoft.com>
 * @date Created 2012-07-26
 */

#ifndef SF1R_CATEGORY_SCORER_H
#define SF1R_CATEGORY_SCORER_H

#include "ProductScorer.h"
#include <boost/shared_ptr.hpp>
#include <vector>

namespace sf1r
{
class GroupLabelLogger;
class SearchManager;

namespace faceted
{
class PropValueTable;
}

class CategoryScorer : public ProductScorer
{
public:
    CategoryScorer(
            const faceted::PropValueTable* categoryValueTable,
            GroupLabelLogger* categoryClickLogger,
            boost::shared_ptr<SearchManager> searchManager,
            const std::vector<std::string>& avgPropNames);

    virtual void pushScore(
            const ProductRankingParam& param,
            ProductScoreMatrix& scoreMatrix);

    void createCategoryScores(ProductRankingParam& param);

private:
    void printScoreReason_(ProductRankingParam& param) const;

    bool getLabelClickCount_(ProductRankingParam& param);

    bool getLabelAvgValue_(
        const std::string& avgPropName,
        ProductRankingParam& param
    );

private:
    const faceted::PropValueTable* categoryValueTable_;

    GroupLabelLogger* categoryClickLogger_;

    boost::shared_ptr<SearchManager> searchManager_;

    const std::vector<std::string>& avgPropNames_;
};

} // namespace sf1r

#endif // SF1R_CATEGORY_SCORER_H
