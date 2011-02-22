/**
 * @file PlmLanguageRanker.cpp
 * @author Ian Yang
 * @date Created <2009-09-24 09:47:22>
 * @date Updated <2010-03-24 15:29:22>
 * @brief
 */
#include "PlmLanguageRanker.h"

#include <util/get.h>

#include <boost/assert.hpp>

#include <stdexcept>
#include <cmath>
#include <numeric>

namespace sf1r {

PlmLanguageRanker::PlmLanguageRanker(
    const TermProximityMeasure* termProximityMeasure,
    float smooth,
    float proximity
)
: termProximityMeasure_(termProximityMeasure)
, smoothArg_(smooth)
, proximityArg_(proximity)
, termProximityArray_()
{}

PlmLanguageRanker::~PlmLanguageRanker()
{
    delete termProximityMeasure_;
    termProximityMeasure_ = 0;
}

PlmLanguageRanker::PlmLanguageRanker(const PlmLanguageRanker& rhs)
: termProximityMeasure_(0)
, smoothArg_(rhs.smoothArg_)
, proximityArg_(rhs.proximityArg_)
{
    if (rhs.termProximityMeasure_)
    {
        termProximityMeasure_ = rhs.termProximityMeasure_->clone();
    }
}

PlmLanguageRanker& PlmLanguageRanker::operator=(const PlmLanguageRanker& rhs)
{
    if (this != &rhs)
    {
        const TermProximityMeasure* cloned = 0;
        if (rhs.termProximityMeasure_)
        {
            cloned = rhs.termProximityMeasure_->clone();
        }

        termProximityMeasure_ = cloned;
        smoothArg_ = rhs.smoothArg_;
        proximityArg_ = rhs.proximityArg_;
    }

    return *this;
}

void PlmLanguageRanker::setTermProximityMeasure(
    const TermProximityMeasure* termProximityMeasure
)
{
    if (termProximityMeasure != termProximityMeasure_)
    {
        const TermProximityMeasure* orig = termProximityMeasure_;
        termProximityMeasure_ = termProximityMeasure;
        delete orig;
    }
}

float PlmLanguageRanker::getScore(
    const RankQueryProperty& queryProperty,
    const RankDocumentProperty& documentProperty
) const
{
    BOOST_ASSERT(termProximityMeasure_);
    if (!termProximityMeasure_)
    {
        throw std::runtime_error(
            "Term Proximity Measure has not been specified"
        );
    }

    float score = 0.0F;
    if (0.0F == queryProperty.getAveragePropertyLength())
    {
        return score;
    }

    std::size_t numTerms = documentProperty.size();
    termProximityArray_.resize(numTerms);

    if (numTerms > 1)
    {
        // calculates term proximity in array parallel to
        // textQuery.termPositions
        termProximityMeasure_->calculate(
            documentProperty,
            termProximityArray_
        );
    }
    else if (numTerms == 1)
    {
        termProximityArray_[0] = 0.0F;
    }

    TermProximityMeasure::array_type::const_iterator termProximity
        = termProximityArray_.begin();
    for (RankDocumentProperty::size_type i = 0;
         i != documentProperty.size(); ++i, ++termProximity)
    {
        float tfInDoc = documentProperty.termFreqAt(i);
        float tfInQuery = queryProperty.termFreqAt(i);

        if(tfInDoc > 0.0F)
        {
            float proximityFactor = *termProximity;

            float collectionMLE =
                queryProperty.totalTermFreqAt(i) /
                queryProperty.getTotalPropertyLength();

            float seenDocProb =
                tfInDoc
                + smoothArg_ * collectionMLE
                + proximityArg_ * proximityFactor;
            // / (smoothArg_ + docTokenCount + proxWeight)

            float unseenDocProb = collectionMLE * smoothArg_;
            // / (smoothArg_ + docTokenCount + proxWeight)

            score += tfInQuery
                     / queryProperty.getQueryLength()
                     * std::log(seenDocProb / unseenDocProb);
        }
    }

    if (score > 0.0F)
    {
        float docTokenCount = documentProperty.docLength();
        float proxWeight =
            proximityArg_
            * std::accumulate(
                termProximityArray_.begin(),
                termProximityArray_.begin() + documentProperty.size(),
                0.0F
            );

        score += std::log(
            (smoothArg_ )
            / (smoothArg_ + docTokenCount + proxWeight)
        );
    } // end - if

    return score;
} // end PlmLanguageRanker::getScore()

PlmLanguageRanker* PlmLanguageRanker::clone() const
{
    return new PlmLanguageRanker(*this);
}

} // namespace sf1r
