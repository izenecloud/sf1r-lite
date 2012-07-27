#ifndef SF1R_COMMON_VALUE_CONVERTER_H
#define SF1R_COMMON_VALUE_CONVERTER_H
/**
 * @file core/common/ValueConverter.h
 * @author Ian Yang
 * @date Created <2010-06-11 17:45:57>
 * @brief Convert between driver::Value and PropertyValue
 */
#include <util/driver/Value.h>
#include <common/PropertyValue.h>

namespace sf1r {
using namespace izenelib;
using namespace izenelib::driver;

class ValueConverter
{
public:
    /// @brief convert @a dirverValue to @a proeprtyValue of specified type @a
    /// dataType.
    static void driverValue2PropertyValue(
        sf1r::PropertyDataType dataType,
        const driver::Value& driverValue,
        PropertyValue& propertyValue
    );

    /// @brief convert @a stringValue to time_t value
    static time_t stringValue2DatetimeValue(const std::string &stringValue);
};

} // namespace sf1r

#endif // SF1R_COMMON_VALUE_CONVERTER_H
