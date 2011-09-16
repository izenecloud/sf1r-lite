#include "TermTypeDetector.h"

namespace sf1r {

PropertyValue TermTypeDetector::propertyValue_;

bool TermTypeDetector::checkUnsignedIntFormat( const std::string & term )
{
    try
    {
        propertyValue_ = boost::lexical_cast< uint64_t >( term );
        return true;
    }
    catch(boost::bad_lexical_cast& e)
    {
        return false;
    }
}

bool TermTypeDetector::checkIntFormat( const std::string & term )
{
    try
    {
        propertyValue_ = boost::lexical_cast< int64_t >( term );
        return true;
    }
    catch(boost::bad_lexical_cast& e)
    {
        return false;
    }
}

bool TermTypeDetector::checkFloatFormat( const std::string & term )
{
    try
    {
        propertyValue_ = boost::lexical_cast< float >( term );
        return true;
    }
    catch(boost::bad_lexical_cast& e)
    {
        return false;
    }
}

bool TermTypeDetector::isTypeMatch(const std::string& term, const sf1r::PropertyDataType& dataType)
{
    switch (dataType)
    {
        case UNSIGNED_INT_PROPERTY_TYPE:
            if (checkUnsignedIntFormat(term))
                return true;
            break;
        case INT_PROPERTY_TYPE:
            if (checkIntFormat(term))
                return true;
            break;
        case FLOAT_PROPERTY_TYPE:
            if (checkFloatFormat(term))
                return true;
            break;
        default:
            break;
    }
    return false;
}

}// namespace sf1r

