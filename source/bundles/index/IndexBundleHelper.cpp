#include "IndexBundleHelper.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>

namespace sf1r
{

void split_string(const izenelib::util::UString& szText, std::list<PropertyType>& out, izenelib::util::UString::EncodingType encoding, char Separator )
{
    izenelib::util::UString str(szText);
    izenelib::util::UString sep(" ",encoding);
    sep[0] = Separator;
    size_t n = 0, nOld=0;
    while (n != izenelib::util::UString::npos)
    {
        n = str.find(sep,n);
        if (n != izenelib::util::UString::npos)
        {
            if (n != nOld)
                out.push_back(str.substr(nOld, n-nOld));
            n += sep.length();
            nOld = n;
        }
    }
    out.push_back(str.substr(nOld, str.length()-nOld));
}

void split_int(const izenelib::util::UString& szText, std::list<PropertyType>& out, izenelib::util::UString::EncodingType encoding, char Separator )
{
    izenelib::util::UString str(szText);
    izenelib::util::UString sep(" ",encoding);
    sep[0] = Separator;
    size_t n = 0, nOld=0;
    while (n != izenelib::util::UString::npos)
    {
        n = str.find(sep,n);
        if (n != izenelib::util::UString::npos)
        {
            if (n != nOld)
            {
                int64_t value = 0;
                try
                {
                    value = boost::lexical_cast< int64_t >( str.substr(nOld, n-nOld) );
                    out.push_back(value);
                }
                catch( const boost::bad_lexical_cast & )
                {}
            }
            n += sep.length();
            nOld = n;
        }
    }

    int64_t value = 0;
    try
    {
        value = boost::lexical_cast< int64_t >( str.substr(nOld, str.length()-nOld) );
        out.push_back(value);
    }
    catch( const boost::bad_lexical_cast & )
    {}
}

void split_float(const izenelib::util::UString& szText, std::list<PropertyType>& out, izenelib::util::UString::EncodingType encoding, char Separator )
{
    izenelib::util::UString str(szText);
    izenelib::util::UString sep(" ",encoding);
    sep[0] = Separator;
    size_t n = 0, nOld=0;
    while (n != izenelib::util::UString::npos)
    {
        n = str.find(sep,n);
        if (n != izenelib::util::UString::npos)
        {
            if (n != nOld)
            {
                float value = 0;
                try
                {
                    value = boost::lexical_cast< float >( str.substr(nOld, n-nOld) );
                    out.push_back(value);
                }
                catch( const boost::bad_lexical_cast & )
                {}
            }
            n += sep.length();
            nOld = n;
        }
    }

    float value = 0;
    try
    {
        value = boost::lexical_cast< float >( str.substr(nOld, str.length()-nOld) );
        out.push_back(value);
    }
    catch( const boost::bad_lexical_cast & )
    {}
}

}
