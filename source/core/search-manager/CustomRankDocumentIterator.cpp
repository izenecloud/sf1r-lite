#include "CustomRankDocumentIterator.h"
#include "FilterDocumentIterator.h"
#include <index-manager/InvertedIndexManager.h>
#include <algorithm> // sort

namespace
{

using namespace sf1r;

FilterDocumentIterator* createFilterDocIterator(CustomRankDocId::DocIdList& docIdList)
{
    boost::shared_ptr<InvertedIndexManager::FilterBitmapT> filterBitmap(
        new InvertedIndexManager::FilterBitmapT);

    std::sort(docIdList.begin(), docIdList.end());

    CustomRankDocId::DocIdList::const_iterator uniqueEnd =
        std::unique(docIdList.begin(), docIdList.end());

    for (CustomRankDocId::DocIdList::const_iterator it = docIdList.begin();
         it != uniqueEnd; ++it)
    {
        filterBitmap->set(*it);
    }

    InvertedIndexManager::FilterTermDocFreqsT* filterTermDocFreqs =
        new InvertedIndexManager::FilterTermDocFreqsT(filterBitmap);

    return new FilterDocumentIterator(filterTermDocFreqs);
}

} // namespace

namespace sf1r
{

CustomRankDocumentIterator::CustomRankDocumentIterator(CustomRankDocId& customRankDocId)
    : defaultScoreDocIterator_(NULL)
{
    if (! customRankDocId.topIds.empty())
    {
        FilterDocumentIterator* includeDocIter =
            createFilterDocIterator(customRankDocId.topIds);

        ORDocumentIterator::add(includeDocIter);
    }

    if (! customRankDocId.excludeIds.empty())
    {
        FilterDocumentIterator* excludeDocIter =
            createFilterDocIterator(customRankDocId.excludeIds);

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
    double result = 0;

    if (defaultScoreDocIterator_)
    {
        result = defaultScoreDocIterator_->score(rankQueryProperties,
                 propertyRankers);
    }

    return result;
}

} // namespace sf1r
