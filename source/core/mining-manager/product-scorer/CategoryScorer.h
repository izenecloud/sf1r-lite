/**
 * @file CategoryScorer.h
 * @brief It gives score according to label click count.
 *
 * For example, the categories of docid [1, 2, 3] are three top labels
 * in order, then their scores would be [3, 2, 1], other docid's score
 * is zero.
 *
 * @author Jun Jiang
 * @date Created 2012-10-26
 */

#ifndef SF1R_CATEGORY_SCORER_H
#define SF1R_CATEGORY_SCORER_H

#include "ProductScorer.h"
#include "../group-manager/PropValueTable.h"
#include <vector>
#include <map>

namespace sf1r
{

class CategoryScorer : public ProductScorer
{
public:
    CategoryScorer(
        const ProductScoreConfig& config,
        const faceted::PropValueTable& categoryValueTable,
        const std::vector<category_id_t>& topLabels);

    virtual score_t score(docid_t docId);

private:
    const faceted::PropValueTable& categoryValueTable_;
    const faceted::PropValueTable::ParentIdTable& parentIdTable_;

    typedef std::map<category_id_t, score_t> CategoryScores;
    CategoryScores categoryScores_;
};

} // namespace sf1r

#endif // SF1R_CATEGORY_SCORER_H
