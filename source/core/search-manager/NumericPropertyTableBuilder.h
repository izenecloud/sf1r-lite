///
/// @file NumericPropertyTableBuilder.h
/// @brief interface to create RTypePropertyTable instance
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @author August Njam Grong <longran1989@gmail.com>
/// @date Created 2011-09-07
/// @date Updated 2012-06-28
///

#ifndef SF1R_RTYPE_TABLE_BUILDER_H
#define SF1R_RTYPE_TABLE_BUILDER_H

#include <common/NumericPropertyTableBase.h>
#include <string>
#include <boost/shared_ptr.hpp>

namespace sf1r
{

class NumericPropertyTableBuilder
{
public:
    virtual ~NumericPropertyTableBuilder() {}

    virtual boost::shared_ptr<NumericPropertyTableBase>& createPropertyTable(
        const std::string& propertyName) = 0;
};

}

#endif
