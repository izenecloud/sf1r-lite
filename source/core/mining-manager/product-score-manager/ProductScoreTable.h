/**
 * @file ProductScoreTable.h
 * @brief a table which stores one score for each doc.
 * @author Jun Jiang
 * @date Created 2012-11-16
 */

#ifndef SF1R_PRODUCT_SCORE_TABLE_H
#define SF1R_PRODUCT_SCORE_TABLE_H

#include "../group-manager/PropSharedLock.h"
#include <common/inttypes.h>
#include <string>
#include <vector>

namespace sf1r
{

class ProductScoreTable : public faceted::PropSharedLock
{
public:
    ProductScoreTable(
        const std::string& dirPath,
        const std::string& scoreTypeName);

    bool open();
    bool flush();

    void resize(std::size_t num);
    void setScore(docid_t docId, score_t score);

    /**
     * @brief get product score for @p docId (has lock version).
     *
     * before calling this function, the caller does not need to acquire
     * the read lock, as it would acquire the lock by itself.
     */
    score_t getScoreHasLock(docid_t docId) const;

    /**
     * @brief get product score for @p docId (no lock version).
     *
     * before calling this function, in order to ensure safe concurrent access,
     * the caller must acquire the read lock first, just like below:
     *
     * <code>
     * ProductScoreTable::ScopedReadLock lock(ProductScoreTable::getMutex());
     * </code>
     */
    score_t getScoreNoLock(docid_t docId) const;

private:
    const std::string dirPath_;

    const std::string scoreTypeName_;

    std::vector<score_t> scores_;
};

} // namespace sf1r

#endif // SF1R_PRODUCT_SCORE_TABLE_H
