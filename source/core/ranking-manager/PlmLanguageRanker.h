#ifndef _PLM_LANGUAGE_RANKER_H_
#define _PLM_LANGUAGE_RANKER_H_
/**
 * @file PlmLanguageRanker.h
 * @author Ian Yang
 * @date Created <2009-09-24 10:28:14>
 * @date Updated <2010-03-24 14:57:43>
 * @brief Extracted from LanguageRanker
 */
#include "RankingDefaultParameters.h"
#include "TermProximityMeasure.h"

#include "PropertyRanker.h"

namespace sf1r {

/**
 * @brief Computes the score in the way s.t. a term in high proximity area
 * will have a high weight.
 */
class PlmLanguageRanker : public PropertyRanker
{
public:
    /**
     * @param termProximityMeasure term proximity calculation delegatee,
     *        it will be deleted when this object is destructed.
     * @param smooth the smoothing parameter for Dirichlet smoothing
     * @param proximity the paramter in BTS-KL model
     */
    explicit PlmLanguageRanker(
            const TermProximityMeasure* termProximityMeasure,
            float smooth = LANGUAGE_RANKER_SMOOTH,
            float proximity = LANGUAGE_RANKER_PROXIMITY
    );

    ~PlmLanguageRanker();

    PlmLanguageRanker(const PlmLanguageRanker& rhs);
    PlmLanguageRanker& operator=(const PlmLanguageRanker& rhs);

    void setTermProximityMeasure(
            const TermProximityMeasure* termProximityMeasure
    );

    float getScoreSVD(
            const RankQueryProperty& queryProperty,
            const RankDocumentProperty& documentProperty,
            const std::vector<double>& queryTF_d,
            const std::vector<double>& queryTF,
            const std::vector<double>& collTF
    ) const;

    float getScore(
            const RankQueryProperty& queryProperty,
            const RankDocumentProperty& documentProperty
    ) const;

    /**
     * @brief term position is required in this ranker
     */
    bool requireTermPosition() const
    {
        return true;
    }

    void setMu(float mu ) {
        smoothArg_ = mu;
    }

    void setLambda(float prox) {
        proximityArg_ = prox;
    }


    PlmLanguageRanker* clone() const;

private:
    const TermProximityMeasure* termProximityMeasure_;

    float smoothArg_;
    float proximityArg_;

    /// @brief intermediate objects, use it to eliminate multiple allocation.
    mutable TermProximityMeasure::array_type termProximityArray_;
}; // end - class LanguageRanker

} // end namespace sf1r

#endif // _PLM_LANGUAGE_RANKER_H_
