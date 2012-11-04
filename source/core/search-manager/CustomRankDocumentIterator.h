/**
 * @file CustomRankDocumentIterator.h
 * @brief an ORDocumentIterator which could iterate customized docs.
 * @author Jun Jiang <jun.jiang@izenesoft.com>
 * @date Created 2012-07-19
 */

#ifndef SF1R_CUSTOM_RANK_DOCUMENT_ITERATOR_H
#define SF1R_CUSTOM_RANK_DOCUMENT_ITERATOR_H

#include "ORDocumentIterator.h"
#include <mining-manager/custom-rank-manager/CustomRankValue.h>

namespace sf1r
{

class CustomRankDocumentIterator : public ORDocumentIterator
{
public:
    CustomRankDocumentIterator(CustomRankDocId& customRankDocId);

    virtual void add(DocumentIterator* pDocIterator);

    virtual double score(
        const std::vector<RankQueryProperty>& rankQueryProperties,
        const std::vector<boost::shared_ptr<PropertyRanker> >& propertyRankers
    );

private:
    DocumentIterator* defaultScoreDocIterator_;
};

} // namespace sf1r

#endif // SF1R_CUSTOM_RANK_DOCUMENT_ITERATOR_H
