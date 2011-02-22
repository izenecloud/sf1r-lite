/**
 * @file    LanguageRanker.h
 * @author  Jinglei Zhao & Dohyun Yun
 * @brief   Given a query, LanuageRanker implements the KL divergence ranking function for ranking.
 * @version 1.1
 *
 * - Copyright   : iZENESoft
 * - Log
 *   - 2008.10.19 Remove index parameter in all interfaces.
 *   - 2008.10.22 Replaced textQuery by SearchKeywordActionItem
 *   - 2008.12.04 Start Applying plmRankingScore()
 *   - 2009.02.26 Applied Ranking Manager Initializer class
 *   - 2009-09-24 Ian Yang <doit.ian@gmail.com>
 *     - Query Adjustment. Rank document item one by one.
 */

#ifndef _LANGUAGERANKER_H_
#define _LANGUAGERANKER_H_

#include "RankingDefaultParameters.h"
#include "PropertyRanker.h"

namespace sf1r {

/**
 * @brief LanuageRanker implements the KL divergence ranking function for
 * ranking.
 *
 * This class also implements a semantic smoothing method to update the query
 * model.
 */
class LanguageRanker : public PropertyRanker {
public:
    /**
     * @param smooth the smoothing parameter for Dirichlet smoothing.
     */
    explicit LanguageRanker(
        float smooth = LANGUAGE_RANKER_SMOOTH
    )
    : smoothArg_(smooth)
    {}

    float getScore(
        const RankQueryProperty& queryProperty,
        const RankDocumentProperty& documentProperty
    ) const;

    LanguageRanker* clone() const;
private:
    float smoothArg_;
}; // end - class LanguageRanker

} // end namespace sf1r

#endif /* _LANGUAGERANKER_H_ */
