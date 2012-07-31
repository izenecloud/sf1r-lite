#include "CustomRankDocumentIterator.h"
#include "FilterDocumentIterator.h"
#include <mining-manager/custom-rank-manager/CustomRankScorer.h>

namespace
{

using namespace sf1r;

FilterDocumentIterator* createFilterDocIterator(const CustomRankValue::DocIdList& docIdList)
{
    boost::shared_ptr<izenelib::am::EWAHBoolArray<uint32_t> > docIdSet(
        new izenelib::am::EWAHBoolArray<uint32_t>());

    for (CustomRankValue::DocIdList::const_iterator it = docIdList.begin();
        it != docIdList.end(); ++it)
    {
        docIdSet->set(*it);
    }

    izenelib::ir::indexmanager::BitMapIterator* bitMapIter =
        new izenelib::ir::indexmanager::BitMapIterator(docIdSet);

    return new FilterDocumentIterator(bitMapIter);
}

} // namespace

namespace sf1r
{

CustomRankDocumentIterator::CustomRankDocumentIterator(CustomRankScorer* customRankScorer)
    : customRankScorer_(customRankScorer)
    , defaultScoreDocIterator_(NULL)
{
    const CustomRankValue& sortCustomValue =
        customRankScorer_->getSortCustomValue();

    if (! sortCustomValue.topIds.empty())
    {
        FilterDocumentIterator* includeDocIter =
            createFilterDocIterator(sortCustomValue.topIds);

        ORDocumentIterator::add(includeDocIter);
    }

    if (! sortCustomValue.excludeIds.empty())
    {
        FilterDocumentIterator* excludeDocIter =
            createFilterDocIterator(sortCustomValue.excludeIds);

        excludeDocIter->setNot(true);
        ORDocumentIterator::add(excludeDocIter);
    }
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
