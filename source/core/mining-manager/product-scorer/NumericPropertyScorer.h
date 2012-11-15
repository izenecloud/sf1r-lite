/**
 * @file NumericPropertyScorer.h
 * @brief It returns the property's numeric value as score.

 * For example, if the property value is 10, then its score is also 10.
 *
 * @author Jun Jiang
 * @date Created 2012-11-14
 */

#ifndef SF1R_NUMERIC_PROPERTY_SCORER_H
#define SF1R_NUMERIC_PROPERTY_SCORER_H

#include "ProductScorer.h"
#include <boost/shared_ptr.hpp>

namespace sf1r
{
class NumericPropertyTableBase;

class NumericPropertyScorer : public ProductScorer
{
public:
    NumericPropertyScorer(
        const ProductScoreConfig& config,
        boost::shared_ptr<NumericPropertyTableBase> numericTable);

    virtual score_t score(docid_t docId);

private:
    boost::shared_ptr<NumericPropertyTableBase> numericTable_;
};

} // namespace sf1r

#endif // SF1R_NUMERIC_PROPERTY_SCORER_H
