#include "TermTypeDetector.h"

namespace sf1r
{

PropertyValue TermTypeDetector::propertyValue_;

bool TermTypeDetector::checkInt32Format(const std::string & term)
{
    try
    {
        propertyValue_ = boost::lexical_cast<int32_t>(term);
        return true;
    }
    catch (boost::bad_lexical_cast& e)
    {
        return false;
    }
}

bool TermTypeDetector::checkInt64Format(const std::string & term)
{
    try
    {
        propertyValue_ = boost::lexical_cast<int64_t>(term);
        return true;
    }
    catch (boost::bad_lexical_cast& e)
    {
        return false;
    }
}

bool TermTypeDetector::checkFloatFormat(const std::string & term)
{
    try
    {
        propertyValue_ = boost::lexical_cast<float>(term);
        return true;
    }
    catch (boost::bad_lexical_cast& e)
    {
        return false;
    }
}

bool TermTypeDetector::isTypeMatch(const std::string& term, const sf1r::PropertyDataType& dataType)
{
    switch (dataType)
    {
        case INT32_PROPERTY_TYPE:
            if (checkInt32Format(term))
                return true;
            break;
        case FLOAT_PROPERTY_TYPE:
            if (checkFloatFormat(term))
                return true;
            break;
        case INT64_PROPERTY_TYPE:
            if (checkInt64Format(term))
                return true;
            break;
        default:
            break;
    }
    return false;
}

}// namespace sf1r
