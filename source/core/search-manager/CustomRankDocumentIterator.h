/**
 * @file CustomRankDocumentIterator.h
 * @brief an ORDocumentIterator which could iterate customized docs. 
 * @author Jun Jiang <jun.jiang@izenesoft.com>
 * @date Created 2012-07-19
 */

#ifndef SF1R_CUSTOM_RANK_DOCUMENT_ITERATOR_H
#define SF1R_CUSTOM_RANK_DOCUMENT_ITERATOR_H

#include "ORDocumentIterator.h"
#include <boost/scoped_ptr.hpp>

namespace sf1r
{
class CustomRankScorer;

class CustomRankDocumentIterator : public ORDocumentIterator
{
public:
    CustomRankDocumentIterator(CustomRankScorer* customRankScorer);

    virtual void add(DocumentIterator* pDocIterator);

    virtual double score(
        const std::vector<RankQueryProperty>& rankQueryProperties,
        const std::vector<boost::shared_ptr<PropertyRanker> >& propertyRankers
    );

private:
    boost::scoped_ptr<CustomRankScorer> customRankScorer_;
    DocumentIterator* defaultScoreDocIterator_;
};

} // namespace sf1r

#endif // SF1R_CUSTOM_RANK_DOCUMENT_ITERATOR_H
