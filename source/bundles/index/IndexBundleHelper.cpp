#include "IndexBundleHelper.h"
#include <ir/index_manager/utility/StringUtils.h>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>

using namespace izenelib::ir::indexmanager;

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

void split_int(const izenelib::util::UString& szText, std::list<PropertyType>& out, izenelib::util::UString::EncodingType encoding, char sep )
{
    std::string str;
    szText.convertString(str, encoding);
    std::size_t n = 0, nOld=0;
    while (n != std::string::npos)
    {
        n = str.find(sep,n);
        if (n != std::string::npos)
        {
            if (n != nOld)
            {
                try
                {
                    std::string tmpStr = str.substr(nOld, n-nOld);
                    boost::algorithm::trim( tmpStr );
                    int64_t value = boost::lexical_cast< int64_t >( tmpStr );
                    out.push_back(value);
                }
                catch(boost::bad_lexical_cast & )
                {
                }
            }
            n += 1;
            nOld = n;
        }
    }

    try
    {
        std::string tmpStr = str.substr(nOld, str.length()-nOld);
        boost::algorithm::trim( tmpStr );
        int64_t value = boost::lexical_cast< int64_t >( tmpStr );
        out.push_back(value);
    }
    catch(boost::bad_lexical_cast & )
    {
    }
}

void split_float(const izenelib::util::UString& szText, std::list<PropertyType>& out, izenelib::util::UString::EncodingType encoding, char sep )
{
    std::string str;
    szText.convertString(str, encoding);
    std::size_t n = 0, nOld=0;
    while (n != std::string::npos)
    {
        n = str.find(sep,n);
        if (n != std::string::npos)
        {
            if (n != nOld)
            {
                try
                {
                    std::string tmpStr = str.substr(nOld, n-nOld);
                    boost::algorithm::trim( tmpStr );
                    float value = boost::lexical_cast< float >( tmpStr );
                    out.push_back(value);
                }
                catch(boost::bad_lexical_cast & )
                {
                }
            }
            n += 1;
            nOld = n;
        }
    }

    try
    {
        std::string tmpStr = str.substr(nOld, str.length()-nOld);
        boost::algorithm::trim( tmpStr );
        float value = boost::lexical_cast< float >( tmpStr );
        out.push_back(value);
    }
    catch(boost::bad_lexical_cast & )
    {
    }
}

}
