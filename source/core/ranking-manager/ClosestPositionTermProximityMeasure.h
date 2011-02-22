#ifndef SF1V5_RANKING_MANAGER_CLOSEST_POSITION_TERM_PROXIMITY_MEASURE_H
#define SF1V5_RANKING_MANAGER_CLOSEST_POSITION_TERM_PROXIMITY_MEASURE_H
/**
 * @file sf1r/ranking-manager/ClosestPositionTermProximityMeasure.h
 * @author Ian Yang
 * @date Created <2009-09-24 15:12:19>
 * @date Updated <2010-03-24 14:59:22>
 */

#include "TermProximityMeasure.h"
#include "RankingDefaultParameters.h"

#include <vector>

namespace sf1r {

class AveClosestPositionTermProximityMeasure
: public TermProximityMeasure
{
public:
    AveClosestPositionTermProximityMeasure(float w = TERM_PROXIMITY_W)
    : w_(w), termProximityMatrix_()
    {}

    void calculate(
        const RankDocumentProperty& documentProperty,
        array_type& result
    ) const;

    AveClosestPositionTermProximityMeasure* clone() const;
private:
    float w_;

    /// @brief intermediate objects, use it to eliminate multiple allocation.
    mutable std::vector<float> termProximityMatrix_;
};

class MinClosestPositionTermProximityMeasure
: public TermProximityMeasure
{
public:
    MinClosestPositionTermProximityMeasure(float w = TERM_PROXIMITY_W)
    : w_(w), termProximityMatrix_()
    {}

    void calculate(
        const RankDocumentProperty& documentProperty,
        array_type& result
    ) const;

    MinClosestPositionTermProximityMeasure* clone() const;
private:
    float w_;

    /// @brief intermediate objects, use it to eliminate multiple allocation.
    mutable std::vector<float> termProximityMatrix_;
};

class MaxClosestPositionTermProximityMeasure
: public TermProximityMeasure
{
public:
    MaxClosestPositionTermProximityMeasure(float w = TERM_PROXIMITY_W)
    : w_(w), termProximityMatrix_()
    {}

    void calculate(
        const RankDocumentProperty& documentProperty,
        array_type& result
    ) const;

    MaxClosestPositionTermProximityMeasure* clone() const;
private:
    float w_;

    /// @brief intermediate objects, use it to eliminate multiple allocation.
    mutable std::vector<float> termProximityMatrix_;
};

} // namespace sf1r

#endif // SF1V5_RANKING_MANAGER_CLOSEST_POSITION_TERM_PROXIMITY_MEASURE_H
