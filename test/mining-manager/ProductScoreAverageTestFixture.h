/**
 * @file ProductScoreAverageTestFixture.h
 * @brief fixture class to create ProductScoreAverage instance.
 * @author Jun Jiang
 * @date Created 2012-11-23
 */

#ifndef SF1R_PRODUCT_SCORE_AVERAGE_TEST_FIXTURE_H
#define SF1R_PRODUCT_SCORE_AVERAGE_TEST_FIXTURE_H

#include <configuration-manager/ProductScoreConfig.h>
#include <mining-manager/product-scorer/ProductScoreAverage.h>
#include <string>
#include <boost/scoped_ptr.hpp>

namespace sf1r
{

class ProductScoreAverageTestFixture
{
public:
    ProductScoreAverageTestFixture();

    /**
     * it adds a list of @c ProductScorerStub instance into @c averageScorer_,
     * each @c ProductScorerStub instance is created with the score and weight
     * from @p scoreList and @p weightList.
     *
     * @param scoreList such as "0.1 0.2 0.3"
     * @param weightList such as "0.3 0.3 0.3"
     */
    void addScorers(
        const std::string& scoreList,
        const std::string& weightList);

protected:
    ProductScoreConfig averageScoreConfig_;
    boost::scoped_ptr<ProductScoreAverage> averageScorer_;
};

} // namespace sf1r

#endif // SF1R_PRODUCT_SCORE_AVERAGE_TEST_FIXTURE_H
