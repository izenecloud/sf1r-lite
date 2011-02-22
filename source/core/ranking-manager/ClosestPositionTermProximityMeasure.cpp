/**
 * @file core/ranking-manager/ClosestPositionTermProximityMeasure.cpp
 * @author Ian Yang
 * @date Created <2009-09-24 15:11:47>
 * @date Updated <2010-03-24 15:25:57>
 */
#include "TermProximityUtil.h"
#include "ClosestPositionTermProximityMeasure.h"

#include <boost/foreach.hpp>

#include <cmath>
#include <algorithm>
#include <numeric>
#include <iostream>

namespace sf1r {

namespace detail {

void makeClosestPositionDistanceMatrix(
    const RankDocumentProperty& documentProperty,
    float w,
    std::vector<float>& resultMatrix
)
{
    std::size_t numTerms = documentProperty.size();

    typedef RankDocumentProperty::size_type size_type;
    std::size_t diagonal = 0;   // diagonal subscripts

    for (size_type outer = 0; outer != documentProperty.size(); ++outer)
    {
        std::size_t horizontal = diagonal;
        std::size_t vertical = diagonal;

        for (size_type inner = outer + 1;
             inner != documentProperty.size(); ++inner)
        {
            float distance = closestPositionDistance(
                documentProperty, outer, inner
            );

            ++horizontal;
            vertical += numTerms;
            float score= std::pow(w, -distance);
            if(distance==1)
                score*=2;
            resultMatrix[horizontal] = resultMatrix[vertical]
                                     = score;
        }

        diagonal += numTerms;
        ++diagonal;
    }
}

} // namespace detail


void AveClosestPositionTermProximityMeasure::calculate(
    const RankDocumentProperty& documentProperty,
    array_type& result
) const
{
    std::size_t numTerms = documentProperty.size();
    termProximityMatrix_.resize(numTerms * numTerms);

    detail::makeClosestPositionDistanceMatrix(
        documentProperty,
        w_,
        termProximityMatrix_
    );

    for(std::size_t i = 0, base = 0; i < numTerms; i++, base += numTerms)
    {
        termProximityMatrix_[base + i] = 0.0F; // not counting itself
        result[i] = std::accumulate(
            termProximityMatrix_.begin() + base,
            termProximityMatrix_.begin() + base + numTerms,
            0.0F
        ) / numTerms;
    }
}

AveClosestPositionTermProximityMeasure*
AveClosestPositionTermProximityMeasure::clone() const
{
    return new AveClosestPositionTermProximityMeasure(*this);
}

void MinClosestPositionTermProximityMeasure::calculate(
    const RankDocumentProperty& documentProperty,
    array_type& result
) const
{
    std::size_t numTerms = documentProperty.size();
    termProximityMatrix_.resize(numTerms * numTerms);

    detail::makeClosestPositionDistanceMatrix(
        documentProperty,
        w_,
        termProximityMatrix_
    );

    for(std::size_t i = 0, base = 0; i < numTerms; i++, base += numTerms)
    {
        termProximityMatrix_[base + i] = (std::numeric_limits<float>::max) ();
        result[i] = *std::min_element(
            termProximityMatrix_.begin() + base,
            termProximityMatrix_.begin() + base + numTerms
        );
    }
}

MinClosestPositionTermProximityMeasure*
MinClosestPositionTermProximityMeasure::clone() const
{
    return new MinClosestPositionTermProximityMeasure(*this);
}

void MaxClosestPositionTermProximityMeasure::calculate(
    const RankDocumentProperty& documentProperty,
    array_type& result
) const
{
    std::size_t numTerms = documentProperty.size();
    termProximityMatrix_.resize(numTerms * numTerms);

    detail::makeClosestPositionDistanceMatrix(
        documentProperty,
        w_,
        termProximityMatrix_
    );

    for(std::size_t i = 0, base = 0; i < numTerms; i++, base += numTerms)
    {
        termProximityMatrix_[base + i] = 0.0F;
        result[i] = *std::max_element(
            termProximityMatrix_.begin() + base,
            termProximityMatrix_.begin() + base + numTerms
        );
    }
}

MaxClosestPositionTermProximityMeasure*
MaxClosestPositionTermProximityMeasure::clone() const
{
    return new MaxClosestPositionTermProximityMeasure(*this);
}

} // namespace sf1r
