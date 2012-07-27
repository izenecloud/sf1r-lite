#include "CustomRankDocumentIterator.h"
#include "FilterDocumentIterator.h"
#include <mining-manager/custom-rank-manager/CustomRankScorer.h>

namespace sf1r
{

CustomRankDocumentIterator::CustomRankDocumentIterator(CustomRankScorer* customRankScorer)
    : customRankScorer_(customRankScorer)
    , defaultScoreDocIterator_(NULL)
{
    boost::shared_ptr<izenelib::am::EWAHBoolArray<uint32_t> > customDocIdSet(
        new izenelib::am::EWAHBoolArray<uint32_t>());

    const CustomRankScorer::ScoreMap& scoreMap =
        customRankScorer_->getScoreMap();

    for (CustomRankScorer::ScoreMap::const_iterator it = scoreMap.begin();
        it != scoreMap.end(); ++it)
    {
        customDocIdSet->set(it->first);
    }

    izenelib::ir::indexmanager::BitMapIterator* bitMapIter = new izenelib::ir::indexmanager::BitMapIterator(customDocIdSet);
    FilterDocumentIterator* filterDocIter = new FilterDocumentIterator(bitMapIter);

    ORDocumentIterator::add(filterDocIter);
}

void CustomRankDocumentIterator::add(DocumentIterator* pDocIterator)
{
    ORDocumentIterator::add(pDocIterator);

    defaultScoreDocIterator_ = pDocIterator;
}

double CustomRankDocumentIterator::score(
    const std::vector<RankQueryProperty>& rankQueryProperties,
    const std::vector<boost::shared_ptr<PropertyRanker> >& propertyRankers
)
{
    double result = customRankScorer_->getScore(currDoc_);

    if (result == 0 && defaultScoreDocIterator_)
    {
        result = defaultScoreDocIterator_->score(
            rankQueryProperties, propertyRankers);
    }

    return result;
}

} // namespace sf1r
