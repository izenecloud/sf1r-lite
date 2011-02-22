#ifndef _PLM_FB_LANGUAGE_RANKER_H_
#define _PLM_FB_LANGUAGE_RANKER_H_
/**
 * @file PlmFbLanguageRanker.h
 * @author Ian Yang
 * @date Created <2009-09-24 10:33:30>
 * @date Updated <2010-03-24 14:57:11>
 * @warn not implemented
 */
#include "RankingDefaultParameters.h"
#include "PlmLanguageRanker.h"

namespace sf1r {

/**
 * @brief The PLM-FB model (proximity language model with proximity feedback).
 */
class PlmFbLanguageRanker : public PlmLanguageRanker
{
public:
    /**
     * @param smooth the smoothing parameter for Dirichlet smoothing
     * @param proximity the paramter in BTS-KL model
     */
    explicit PlmFbLanguageRanker(
        const TermProximityMeasure* termProximityMeasure,
        float smooth = LANGUAGE_RANKER_SMOOTH,
        float proximity = LANGUAGE_RANKER_PROXIMITY
    )
    : PlmLanguageRanker(termProximityMeasure, smooth, proximity)
    {}

    float getScore(
        const RankQueryProperty& queryProperty,
        const RankDocumentProperty& documentProperty
    ) const;

    PlmFbLanguageRanker* clone() const;
}; // end - class PlmFbLanguageRanker

} // end namespace sf1r

#endif // _PLM_FB_LANGUAGE_RANKER_H_
