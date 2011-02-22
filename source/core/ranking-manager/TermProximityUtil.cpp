/**
 * @file core/ranking-manager/TermProximityUtil.cpp
 * @author Ian Yang
 * @date Created <2009-09-24 14:48:34>
 * @date Updated <2010-03-24 15:23:15>
 */
#include "TermProximityUtil.h"

#include <boost/assert.hpp>
#include <iostream>

namespace sf1r {

unsigned closestPositionDistance(
    const RankDocumentProperty& documentProperty,
    RankDocumentProperty::size_type i,
    RankDocumentProperty::size_type j
)
{
    BOOST_ASSERT(i < documentProperty.size());
    BOOST_ASSERT(j < documentProperty.size());

    unsigned distance = documentProperty.docLength();

    typedef RankDocumentProperty::const_position_iterator iterator;

    iterator it1 = documentProperty.termPositionsBeginAt(i);
    iterator itEnd1 = documentProperty.termPositionsEndAt(i);
    iterator it2 = documentProperty.termPositionsBeginAt(j);
    iterator itEnd2 = documentProperty.termPositionsEndAt(j);

//    while(it1!=itEnd1)
//    {
//        std::cout<<*it1<<":";
//        it1++;
//    }
//    std::cout<<std::endl;
//    it1 = documentProperty.termPositionsBeginAt(i);
//    while(it2!=itEnd2)
//    {
//        std::cout<<*it2<<":";
//        it2++;
//    }
//    std::cout<<std::endl;
//    it2 = documentProperty.termPositionsBeginAt(j);

    while (it1 != itEnd1 && it2 != itEnd2)
    {
        unsigned currentDistance = 0;
        if (*it1 < *it2)
        {
            currentDistance = *it2 - *it1;
            ++it1;
        }
        else if (*it2 < *it1)
        {
            currentDistance = *it1 - *it2;
            ++it2;
        }
        else // rarely *it1 == *it2
        {
            return 1;
        }

        if (currentDistance < distance)
        {
            distance = currentDistance;
        }
    }
//    std::cout<<"!!!!!!!!!!"<<distance<<std::endl;

    return distance;
}

} // namespace sf1r
