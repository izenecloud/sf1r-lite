/**
 * @file RecommendMatrix.h
 * @brief interface class to update recommend matrix
 * @author Jun Jiang
 * @date 2012-01-19
 */

#ifndef RECOMMEND_MATRIX_H
#define RECOMMEND_MATRIX_H

#include "../common/RecTypes.h"

#include <list>

namespace sf1r
{

class RecommendMatrix
{
public:
    virtual ~RecommendMatrix() {}

    virtual void update(
        const std::list<itemid_t>& oldItems,
        const std::list<itemid_t>& newItems
    ) = 0;
};

} // namespace sf1r

#endif // RECOMMEND_MATRIX_H
