/**
 * \file SearchingEnumerator.h
 * \brief 
 * \date Feb 2, 2012
 * \author Xin Liu
 */

#ifndef _SEARCHINGENUMERATOR_H_
#define _SEARCHINGENUMERATOR_H_

namespace sf1r {

namespace SearchingMode {

enum SearchingModeType{
    DefaultSearchingMode = 0,
    OR,
    VERBOSE,
    KNN,
    NotUseSearchingMode
}; //end - enum SearchingModeType

} //end - namespace SearchingMode

} //end - namespace sf1r

#endif /* SEARCHINGENUMERATOR_H_ */
