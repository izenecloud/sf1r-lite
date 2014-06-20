#include "TermTypeDetector.h"

namespace sf1r
{

PropertyValue TermTypeDetector::propertyValue_;

bool TermTypeDetector::isTypeMatch(const std::string& term, const sf1r::PropertyDataType& dataType)
{
    switch (dataType)
    {
        case INT32_PROPERTY_TYPE: return checkNumericFormat<int32_t>(term);
        case FLOAT_PROPERTY_TYPE: return checkNumericFormat<float>(term);
        case INT64_PROPERTY_TYPE: return checkNumericFormat<int64_t>(term);
        case DOUBLE_PROPERTY_TYPE: return checkNumericFormat<double>(term);
        default: return false;
    }
}

}// namespace sf1r
