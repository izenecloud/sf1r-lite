/**
 * \file SearchingEnumerator.h
 * \brief
 * \date Feb 2, 2012
 * \author Xin Liu
 */

#ifndef _SEARCHINGENUMERATOR_H_
#define _SEARCHINGENUMERATOR_H_

namespace sf1r
{

struct SearchingMode
{
    enum SearchingModeType
    {
        DefaultSearchingMode = 0,
        OR,
        WAND,
        VERBOSE,
        KNN,
        SUFFIX_MATCH,
        NotUseSearchingMode
    }; //end - enum SearchingModeType

    enum SuffixMatchFilterMode
    {
        DefaultFilterMode = 0,
        OR_Filter = 0,
        AND_Filter = 1,
        END_Filter
    };
}; //end - struct SearchingMode

} //end - namespace sf1r

#endif /* SEARCHINGENUMERATOR_H_ */
