/**
 * @file CategoryBoostingScorer.h
 * @brief when the doc belongs to the boosting cateogry, it gives score 1;
 *        otherwise, it gives score 0;
 * @author Jun Jiang <jun.jiang@izenesoft.com>
 * @date Created 2012-05-17
 */

#ifndef SF1R_CATEGORY_BOOSTING_SCORER
#define SF1R_CATEGORY_BOOSTING_SCORER

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
struct GroupPropParam;
}

class CategoryBoostingScorer : public ProductScorer
{
public:
    CategoryBoostingScorer(
        const faceted::PropValueTable* categoryValueTable,
        GroupLabelLogger* categoryClickLogger,
        boost::shared_ptr<SearchManager> searchManager,
        const std::string& boostingSubProp
    );

    virtual void pushScore(
        const ProductRankingParam& param,
        ProductScoreMatrix& scoreMatrix
    );

    void selectBoostingCategory(ProductRankingParam& param);

private:
    enum BOOSTING_REASON
    {
        BOOSTING_SELECT_LABEL = 0,
        BOOSTING_FREQ_LABEL,
        BOOSTING_MAX_AVG_LABEL,
        BOOSTING_REASON_NUM
    };

    void printBoostingLabel_(
        const std::string& query,
        category_id_t boostLabel,
        BOOSTING_REASON reason
    ) const;

    category_id_t getFirstSelectLabel_(const faceted::GroupParam& groupParam);

    category_id_t getFreqLabel_(const std::string& query);

    category_id_t getMaxAvgLabel_(const std::vector<docid_t>& docIds);

private:
    const faceted::PropValueTable* categoryValueTable_;
    GroupLabelLogger* categoryClickLogger_;
    boost::shared_ptr<SearchManager> searchManager_;

    const std::string& boostingSubProp_;

    std::vector<std::string> boostingReasons_;
};

} // namespace sf1r

#endif // SF1R_CATEGORY_BOOSTING_SCORER
