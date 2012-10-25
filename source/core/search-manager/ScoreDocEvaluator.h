/**
 * @file ScoreDocEvaluator.h
 * @brief evaluate the score values for ScoreDoc.
 * @author Jun Jiang
 * @date Created 2012-10-25
 */

#ifndef SF1R_SCORE_DOC_EVALUATOR_H
#define SF1R_SCORE_DOC_EVALUATOR_H

#include "CustomRanker.h"
#include <vector>
#include <boost/shared_ptr.hpp>

namespace sf1r
{
class DocumentIterator;
class Sorter;
class RankQueryProperty;
class PropertyRanker;
struct ScoreDoc;

class ScoreDocEvaluator
{
public:
    ScoreDocEvaluator(
        DocumentIterator* scoreDocIterator,
        Sorter* sorter,
        const std::vector<RankQueryProperty>& rankQueryProps,
        const std::vector<boost::shared_ptr<PropertyRanker> >& propRankers,
        CustomRankerPtr customRanker);

    void evaluate(ScoreDoc& scoreDoc);

private:
    const bool isNeedScore_;

    DocumentIterator* scoreDocIterator_;
    const std::vector<RankQueryProperty>& rankQueryProps_;
    const std::vector<boost::shared_ptr<PropertyRanker> >& propRankers_;

    CustomRankerPtr customRanker_;
};

} // namespace sf1r

#endif // SF1R_SCORE_DOC_EVALUATOR_H
