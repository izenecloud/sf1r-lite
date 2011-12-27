#include <common/SFLogger.h>
#include "TokenizerConfigUnit.h"


using namespace std;
using namespace izenelib::util;

namespace sf1r
{
int readTokenizerUCS2Char( const string& in, UCS2Char* ret )
{
    string tmp = in;
    if ( tmp.length() >= 2 && ( tmp[1] == 'x' || tmp[1] == 'X' ) )
    {
        if ( tmp[0] != '0' )
            return 0;
        tmp = tmp.substr( 2 );
    }

    if ( tmp.empty() )
        return 0;
    for ( size_t i = 0; i < tmp.length(); ++i )
    {
        char c = tmp[i];
        if ( (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') )
            continue;
        return 0;
    }
    return sscanf( tmp.c_str(), "%hx", ret );
}

izenelib::util::UString TokenizerConfigUnit::getChars() const
{
    size_t startIndex=0, endIndex=0;

    UString ret( value_, UString::UTF_8 );       // TODO: need to get system encoding

    while ( (startIndex = code_.find_first_not_of( " ,\t", startIndex )) != std::string::npos )
    {
        endIndex = code_.find_first_of( " ,\t", startIndex + 1 );

        if ( endIndex == string::npos )
        {
            endIndex = code_.length();
        }

        std::string sub = code_.substr( startIndex, endIndex - startIndex );

        size_t range = sub.find( '~' );

        if ( range == 0 || range == (sub.length() -1) )
        {
            LOG(WARNING) << "Invalid code value "<<code_<<" in tokenizer "<<id_;
            return ret;
        }

        if ( range == string::npos )
        {
            UCS2Char uchar = 0;
            if ( readTokenizerUCS2Char( sub.c_str(), &uchar ) == 0)
            {
                LOG(WARNING) << "Invalid code value "<<sub<<" among "<<code_<<" in tokenizer "<<id_;
                return ret;
            }
            ret += uchar;
        }
        else
        {
            UCS2Char start = 0, end = 0;
            if ( readTokenizerUCS2Char( sub.substr(0, range).c_str(), &start ) == 0 )
            {
                LOG(WARNING) << "Invalid code value "<<sub.substr(0, range)<<" among "<<code_<<" in tokenizer "<<id_;
                return ret;
            }

            if ( readTokenizerUCS2Char( sub.substr(range+1, sub.length()-range).c_str(), &end ) == 0 )
            {
                LOG(WARNING) << "Invalid code value "<< sub.substr(range+1, sub.length()-range)
                    <<" among "<<code_<<" in tokenizer "<<id_;
                return ret;
            }

            for ( ; start <= end; start++ )
            {
                ret += start;
            }
        }

        startIndex = endIndex + 1;
    }

    return ret;
}

}
