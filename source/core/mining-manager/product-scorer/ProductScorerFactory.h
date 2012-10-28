/**
 * @file ProductScorerFactory.h
 * @brief create ProductScorer instance.
 * @author Jun Jiang
 * @date Created 2012-10-28
 */

#ifndef SF1R_PRODUCT_SCORER_FACTORY_H
#define SF1R_PRODUCT_SCORER_FACTORY_H

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

namespace sf1r
{
class ProductScorer;
class DocumentIterator;
class RankQueryProperty;
class PropertyRanker;
class CustomRankManager;
class GroupLabelLogger;

namespace faceted
{
class PropValueTable;
class PropSharedLockSet;
}

class ProductScorerFactory
{
public:
    ProductScorerFactory(
        CustomRankManager* customRankManager,
        GroupLabelLogger* categoryClickLogger,
        const faceted::PropValueTable* categoryValueTable);

    ProductScorer* createScorer(
        const std::string& query,
        faceted::PropSharedLockSet& propSharedLockSet,
        DocumentIterator* scoreDocIterator,
        const std::vector<RankQueryProperty>& rankQueryProps,
        const std::vector<boost::shared_ptr<PropertyRanker> >& propRankers);

private:
    ProductScorer* createCustomScorer_(const std::string& query);

    ProductScorer* createCategoryScorer_(
        const std::string& query,
        faceted::PropSharedLockSet& propSharedLockSet);

    ProductScorer* createRelevanceScorer_(
        DocumentIterator* scoreDocIterator,
        const std::vector<RankQueryProperty>& rankQueryProps,
        const std::vector<boost::shared_ptr<PropertyRanker> >& propRankers);

private:
    CustomRankManager* customRankManager_;

    GroupLabelLogger* categoryClickLogger_;

    const faceted::PropValueTable* categoryValueTable_;
};

} // namespace sf1r

#endif // SF1R_PRODUCT_SCORER_FACTORY_H
