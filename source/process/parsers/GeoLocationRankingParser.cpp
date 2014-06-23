#include "GeoLocationRankingParser.h"

#include <common/ValueConverter.h>
#include <common/BundleSchemaHelpers.h>
#include <common/Keys.h>

namespace sf1r {

using driver::Keys;

bool GeoLocationRankingParser::parse(const Value& geoLocationValue)
{
    clearMessages();
    property_.clear();

    if (nullValue(geoLocationValue))
    {
        return true;
    }

    std::string property = asString(geoLocationValue[Keys::property]);

    PropertyConfig propertyConfig;
    propertyConfig.setName(property);

    if (!getPropertyConfig(indexSchema_, propertyConfig))
    {
        error() = "Unknown property in geolocation[property]: " + property;
        return false;
    }

    if (propertyConfig.bIndex_ && propertyConfig.bFilter_ && propertyConfig.bRange_
            && propertyConfig.propertyType_ == DOUBLE_PROPERTY_TYPE)
    {
        property_ = property;
    }
    else
    {
        warning() = "Ignore geolocation[property]: " + property;
    }

    double latitude = asDouble(geoLocationValue[Keys::latitude]);
    double longitude = asDouble(geoLocationValue[Keys::longitude]);

    reference_ = std::make_pair(longitude, latitude);

    return true;
}

} // namespace sf1r
