/**
 * @file CategoryClassifyScorer.h
 * @brief It gives score according to classified category.
 *
 * For example, if the query is classified to category c1 and c2, their category
 * score is s1 and s2, in search result, if the title is classified to c1,
 * then its score would be s1.
 */

#ifndef SF1R_CATEGORY_CLASSIFY_SCORER_H
#define SF1R_CATEGORY_CLASSIFY_SCORER_H

#include "ProductScorer.h"
#include <map>

namespace sf1r
{
class CategoryClassifyTable;

class CategoryClassifyScorer : public ProductScorer
{
public:
    typedef std::map<std::string, double> CategoryScoreMap;

    CategoryClassifyScorer(
        const ProductScoreConfig& config,
        const CategoryClassifyTable& categoryClassifyTable,
        const CategoryScoreMap& categoryScoreMap);

    virtual score_t score(docid_t docId);

private:
    const CategoryClassifyTable& categoryClassifyTable_;

    const CategoryScoreMap categoryScoreMap_;

    bool hasGoldCategory_;
};

} // namespace sf1r

#endif // SF1R_CATEGORY_CLASSIFY_SCORER_H
