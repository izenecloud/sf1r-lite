/**
 * @file NumericPropertyScorerTestFixture.h
 * @brief fixture class to test NumericPropertyScorer functions.
 * @author Jun Jiang
 * @date Created 2012-11-26
 */

#ifndef SF1R_NUMERIC_PROPERTY_SCORER_TEST_FIXTURE_H
#define SF1R_NUMERIC_PROPERTY_SCORER_TEST_FIXTURE_H

#include <configuration-manager/ProductScoreConfig.h>
#include <mining-manager/product-scorer/NumericPropertyScorer.h>
#include <common/NumericPropertyTableBase.h>
#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

namespace sf1r
{

class NumericPropertyScorerTestFixture
{
public:
    NumericPropertyScorerTestFixture();

    /**
     * for example, given @p numericList of "0.1 0.2 0.3",
     * in @c numericTable_, it assigns numeric value 0.1 to docid 1,
     * value 0.2 to docid 2, and value 0.3 to docid 3.
     */
    void setNumericValue(const std::string& numericList);

    void setNumericValue(docid_t docId, int numericValue);

    void createScorer();

    void checkScore(docid_t docId, score_t gold);

protected:
    ProductScoreConfig numericScoreConfig_;
    boost::scoped_ptr<NumericPropertyScorer> numericScorer_;
    boost::shared_ptr<NumericPropertyTableBase> numericTable_;
};

} // namespace sf1r

#endif // SF1R_NUMERIC_PROPERTY_SCORER_TEST_FIXTURE_H
