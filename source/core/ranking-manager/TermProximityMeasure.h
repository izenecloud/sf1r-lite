#ifndef SF1V5_RANKING_MANAGER_TERM_PROXIMITY_MEASURE_H
#define SF1V5_RANKING_MANAGER_TERM_PROXIMITY_MEASURE_H
/**
 * @file core/ranking-manager/TermProximityMeasure.h
 * @author Ian Yang
 * @date Created <2009-09-27 15:10:18>
 * @date Updated <2010-03-24 14:58:43>
 */
#include "RankDocumentProperty.h"

#include <vector>

namespace sf1r {

class TermProximityMeasure
{
public:
    typedef std::vector<float> array_type;

    virtual ~TermProximityMeasure()
    {}

    virtual void calculate(
        const RankDocumentProperty& documentProperty,
        array_type& result
    ) const = 0;

    virtual TermProximityMeasure* clone() const = 0;
};

} // namespace sf1r

#endif // SF1V5_RANKING_MANAGER_TERM_PROXIMITY_MEASURE_H
