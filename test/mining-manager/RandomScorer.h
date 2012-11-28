/**
 * @file RandomScorer.h
 * @brief generate random score in range [0, 1)
 * @author Jun Jiang
 * @date Created 2012-11-20
 */

#ifndef SF1R_RANDOM_SCORER_H
#define SF1R_RANDOM_SCORER_H

#include <mining-manager/product-scorer/ProductScorer.h>
#include <boost/random.hpp>

namespace sf1r
{

class RandomScorer : public ProductScorer
{
public:
    typedef boost::mt19937 Engine;

    RandomScorer(
        const ProductScoreConfig& config,
        Engine& engine);

    virtual score_t score(docid_t docId);

private:
    typedef boost::uniform_01<score_t> Distribution;
    typedef boost::variate_generator<Engine&, Distribution> Generator;

    Generator generator_;
};

} // namespace sf1r

#endif // SF1R_RANDOM_SCORER_H
