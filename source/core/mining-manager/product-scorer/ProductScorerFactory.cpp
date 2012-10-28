#include "ProductScorerFactory.h"
#include "ProductScoreSum.h"
#include "CustomScorer.h"
#include "CategoryScorer.h"
#include "RelevanceScorer.h"
#include "../custom-rank-manager/CustomRankManager.h"
#include "../group-label-logger/GroupLabelLogger.h"
#include "../group-manager/PropSharedLockSet.h"
#include <memory> // auto_ptr

using namespace sf1r;

namespace
{
/**
 * in order to make the category score less than 10 (the minimum custom
 * score), we would select at most 9 top labels.
 */
const score_t kTopLabelLimit = 9;
}

ProductScorerFactory::ProductScorerFactory(
    CustomRankManager* customRankManager,
    GroupLabelLogger* categoryClickLogger,
    const faceted::PropValueTable* categoryValueTable)
    : customRankManager_(customRankManager)
    , categoryClickLogger_(categoryClickLogger)
    , categoryValueTable_(categoryValueTable)
{
}

ProductScorer* ProductScorerFactory::createScorer(
    const std::string& query,
    faceted::PropSharedLockSet& propSharedLockSet,
    DocumentIterator* scoreDocIterator,
    const std::vector<RankQueryProperty>& rankQueryProps,
    const std::vector<boost::shared_ptr<PropertyRanker> >& propRankers)
{
    std::auto_ptr<ProductScoreSum> scoreSum(new ProductScoreSum);
    ProductScorer* scorer = NULL;

    if ((scorer = createCustomScorer_(query)))
    {
        scoreSum->addScorer(scorer);
    }

    if ((scorer = createCategoryScorer_(query, propSharedLockSet)))
    {
        scoreSum->addScorer(scorer);
    }

    if ((scorer = createRelevanceScorer_(scoreDocIterator,
                                         rankQueryProps, propRankers)))
    {
        scoreSum->addScorer(scorer);
    }

    return scoreSum.release();
}

ProductScorer* ProductScorerFactory::createCustomScorer_(const std::string& query)
{
    if (customRankManager_ == NULL)
        return NULL;

    CustomRankDocId customDocId;
    bool result = customRankManager_->getCustomValue(query, customDocId);

    if (result && !customDocId.topIds.empty())
        return new CustomScorer(customDocId.topIds);

    return NULL;
}

ProductScorer* ProductScorerFactory::createCategoryScorer_(
    const std::string& query,
    faceted::PropSharedLockSet& propSharedLockSet)
{
    if (categoryClickLogger_ == NULL ||
        categoryValueTable_ == NULL)
        return NULL;

    std::vector<faceted::PropValueTable::pvid_t> topLabels;
    std::vector<int> topFreqs;
    bool result = categoryClickLogger_->getFreqLabel(query, kTopLabelLimit,
                                                     topLabels, topFreqs);

    if (result && !topLabels.empty())
    {
        propSharedLockSet.insertSharedLock(categoryValueTable_);
        return new CategoryScorer(*categoryValueTable_, topLabels);
    }

    return NULL;
}

ProductScorer* ProductScorerFactory::createRelevanceScorer_(
    DocumentIterator* scoreDocIterator,
    const std::vector<RankQueryProperty>& rankQueryProps,
    const std::vector<boost::shared_ptr<PropertyRanker> >& propRankers)
{
    return new RelevanceScorer(scoreDocIterator, rankQueryProps, propRankers);
}
