/**
 * @file ProductScoreReader.h
 * @brief a ProductScorer which reads ProductScoreTable.
 * @author Jun Jiang
 * @date Created 2012-11-16
 */

#ifndef SF1R_PRODUCT_SCORE_READER_H
#define SF1R_PRODUCT_SCORE_READER_H

#include "../product-scorer/ProductScorer.h"
#include "../group-manager/PropSharedLock.h"

namespace sf1r
{
class ProductScoreTable;

class ProductScoreReader : public ProductScorer
{
public:
    ProductScoreReader(
        const ProductScoreConfig& config,
        const ProductScoreTable& scoreTable);

    virtual score_t score(docid_t docId);

private:
    const ProductScoreTable& productScoreTable_;

    faceted::PropSharedLock::ScopedReadLock lock_;
};

} // namespace sf1r

#endif // SF1R_PRODUCT_SCORE_READER_H
