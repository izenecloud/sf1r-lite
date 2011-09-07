///
/// @file NumericPropertyTableBuilder.h
/// @brief interface to create NumericPropertyTable instance
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-09-07
///

#ifndef SF1R_NUMERIC_PROPERTY_TABLE_BUILDER_H
#define SF1R_NUMERIC_PROPERTY_TABLE_BUILDER_H

#include <string>

namespace sf1r
{

class NumericPropertyTable;

class NumericPropertyTableBuilder
{
public:
    virtual ~NumericPropertyTableBuilder() {}

    virtual NumericPropertyTable* createPropertyTable(const std::string& propertyName) = 0;
};

}

#endif 
