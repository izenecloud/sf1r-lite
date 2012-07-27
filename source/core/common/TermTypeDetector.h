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
#include "PropertyValue.h"

#include <boost/lexical_cast.hpp>

using namespace std;

namespace sf1r{

class TermTypeDetector
{
public:
    TermTypeDetector() {}
    ~TermTypeDetector() {}
public:
    /// @brief  Checks if a given string can be converted to 32-bit integer form
    static bool checkInt32Format(const string & term);

    /// @brief  Checks if a given string can be converted to 64-bit integer form
    static bool checkInt64Format(const string & term);

    /// @brief  Checks if a given string can be converted to float form
    static bool checkFloatFormat(const string & term);

    static bool isTypeMatch(const std::string & term, const sf1r::PropertyDataType& dataType);

public:
    static PropertyValue propertyValue_;
};

}

#endif /* TERM_TYPE_DETECTOR_H_ */
