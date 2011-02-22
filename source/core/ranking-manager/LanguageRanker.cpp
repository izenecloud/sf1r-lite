/**
 * @file    LanguageRanker.cpp
 * @author  Jinglei Zhao & Dohyun Yun
 * @brief   Given a query, LanuageRanker implements the KL divergence ranking
 * function for ranking.
 * @version 1.1
 *
 * - Copyright   : iZENESoft
 * - Log
 *   - 2009-09-24 Ian Yang <doit.ian@gmail.com>
 *     - Query Adjustment. Rank document item one by one.
 */
#include "LanguageRanker.h"

#include <util/get.h>

#include <cmath>

namespace sf1r {

float LanguageRanker::getScore(
    const RankQueryProperty& queryProperty,
    const RankDocumentProperty& documentProperty
) const
{
    float score = 0.0F;
    if (0 == queryProperty.getTotalPropertyLength())
    {
        return score;
    }

    float docTokenCount = documentProperty.docLength();

    for (std::size_t i = 0; i != queryProperty.size(); ++i)
    {
        float tfInDoc = documentProperty.termFreqAt(i);
        float tfInQuery = queryProperty.termFreqAt(i);

        if(tfInDoc > 0.0F)
        {
            float collectionMLE =
                queryProperty.totalTermFreqAt(i) /
                queryProperty.getTotalPropertyLength();

            float seenDocProb = (tfInDoc + smoothArg_ * collectionMLE);
            // / (smoothArg_ + docTokenCount);

            float unseenDocProb = collectionMLE * smoothArg_;
            // / (smoothArg_ + docTokenCount);

            score += tfInQuery / queryProperty.getQueryLength()
                     * std::log(seenDocProb / unseenDocProb);

        } // end if(tfInDoc > 0.0F)
    }

    if (score > 0.0F)
    {
        score += std::log(smoothArg_ / (smoothArg_ + docTokenCount));
    }

    return score;
} // end LanguageRanker::getScore

LanguageRanker* LanguageRanker::clone() const
{
    return new LanguageRanker(*this);
}

} // namespace sf1r
