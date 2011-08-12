/**
 * \file TermTypeDetector.h
 * \brief 
 * \date Aug 9, 2011
 * \author Xin Liu
 */

#ifndef TERM_TYPE_DETECTOR_H_
#define TERM_TYPE_DETECTOR_H_


#include <string>
#include "type_defs.h"

using namespace std;

namespace sf1r{

class TermTypeDetector
{
public:
    TermTypeDetector(){}
    ~TermTypeDetector(){}
public:
    /// @brief  Checks if a given string can be converted to unsigned integer form
    static bool checkUnsignedIntFormat( const string & term );

    /// @brief  Checks if a given string can be converted to unsigned integer form
    static bool checkIntFormat( const string & term );

    /// @brief  Checks if a given string can be converted to float form
    static bool checkFloatFormat( const string & term );

    static bool isTypeMatch( const std::string & term, const sf1r::PropertyDataType& dataType);
};

}

#endif /* TERM_TYPE_DETECTOR_H_ */
