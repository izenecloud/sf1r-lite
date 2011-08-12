#include "TermTypeDetector.h"

namespace sf1r {

bool TermTypeDetector::checkUnsignedIntFormat( const std::string & term )
{
    std::size_t pos = term.find_first_not_of( "0123456789", 0 );
    if( pos != std::string::npos )
    {
        return false;
    }
    return true;
}

bool TermTypeDetector::checkIntFormat( const std::string & term )
{
    std::size_t pos = term.find_first_not_of( "0123456789", 0 );
    if( pos != std::string::npos )
    {
        //the value is a negative number
        if( pos == 0 && term[pos] == '-' )
        {
            pos = term.find_first_not_of( "0123456789", pos+1 );
            if( pos == string::npos )
                return true;
            else
                return false;
        }
        return false;
     }
     return true;
}

bool TermTypeDetector::checkFloatFormat( const std::string & term )
{
    std::size_t pos = 0, pos_end = 0;
    int dot_num = 0;
    while ( pos != std::string::npos )
    {
        pos = term.find(".", pos);
        if ( pos == 0 )
            return false;
        else if (pos != std::string::npos)
        {
            dot_num++;
            pos_end = pos;
            pos++;
        }
    }
    if (dot_num > 1 || pos_end == term.length() - 1)
    {
        return false;
    }

    pos = term.find_first_not_of( ".0123456789", 0 );
    if( pos != std::string::npos )
    {
         //the value is a negative number
         if( pos == 0 && term[pos] == '-' )
         {
             pos = term.find_first_not_of( ".0123456789", pos+1 );
             if( pos == string::npos )
                 return true;
             else
                 return false;
         }
         return false;
     }
     return true;
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

