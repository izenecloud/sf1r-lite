#include "product_price.h"

#include <sstream>
#include <boost/operators.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>

using namespace sf1r;

ProductPrice::ProductPrice():value(-1.0, -1.0)
{
}

ProductPrice::ProductPrice(ProductPriceType a, ProductPriceType b): value(a,b)
{
    Check_();
}

ProductPrice& ProductPrice::operator+=(const ProductPrice& a)
{
    if (!Valid())
    {
        if (a.Valid())
        {
            value = a.value;
        }
    }
    else
    {
        if (a.Valid())
        {
            value.first = std::min(value.first, a.value.first);
            value.second = std::max(value.second, a.value.second);
        }
    }
    return *this;
}

bool ProductPrice::operator==(const ProductPrice& b) const
{
    return Valid() && b.Valid() && value == b.value;
}

bool ProductPrice::Parse(const izenelib::util::UString& ustr)
{
    std::string str;
    ustr.convertString(str, izenelib::util::UString::UTF_8);
    return Parse(str);
}

bool ProductPrice::Parse(const std::string& str)
{
    try
    {
        ProductPriceType p = boost::lexical_cast<ProductPriceType>(str);
        value.first = p;
        value.second = p;
    }
    catch (boost::bad_lexical_cast &)
    {
        static const char sep[] = {'-', '~', ','};
        static const uint32_t len = sizeof(sep) / sizeof(char);
        bool found = false;
        for (uint32_t i = 0; i < len; i++)
        {
            if (checkSeparatorType_(str, sep[i]) )
            {
                found = true;
                if (!split_float_(str, sep[i]))
                {
                    Reset_();
                    return false;
                }
                break;
            }
        }
        if (!found)
        {
            Reset_();
            return false;
        }
    }
    return true;
}

std::string ProductPrice::ToString() const
{
    if (!Valid()) return "";
    std::stringstream ss;
    if (value.first == value.second)
    {
        ss << value.first;
    }
    else
    {
        ss << value.first << "-" << value.second;
    }
    return ss.str();
}

izenelib::util::UString ProductPrice::ToUString() const
{
    return izenelib::util::UString(ToString(), izenelib::util::UString::UTF_8);
}

bool ProductPrice::Valid() const
{
    return value.first >= 0.0 && value.second >= 0.0;
}

void ProductPrice::Check_()
{
    if (value.first > value.second)
    {
        //invalid
        std::swap(value.first, value.second);
    }
}

void ProductPrice::Reset_()
{
    value.first = -1.0;
    value.second = -1.0;
}

bool ProductPrice::checkSeparatorType_(const std::string& propertyValueStr, char separator)
{
    size_t n = propertyValueStr.find(separator,0);
    if (n != std::string::npos)
        return true;
    return false;
}

bool ProductPrice::split_float_(const std::string& str, char sep)
{
    std::size_t n = 0, nOld = 0;
    std::vector<ProductPriceType> value_list;
    while (n != std::string::npos)
    {
        n = str.find(sep, n);
        if (n != std::string::npos)
        {
            if (n != nOld)
            {
                try
                {
                    std::string tmpStr = str.substr(nOld, n - nOld);
                    boost::algorithm::trim(tmpStr);
                    ProductPriceType p = boost::lexical_cast<ProductPriceType>(tmpStr);
                    value_list.push_back(p);
                }
                catch (boost::bad_lexical_cast &)
                {
                    return false;
                }
            }
            nOld = ++n;
        }
    }

    try
    {
        std::string tmpStr = str.substr(nOld, str.length() - nOld);
        boost::algorithm::trim(tmpStr);
        ProductPriceType p = boost::lexical_cast<ProductPriceType>(tmpStr);
        value_list.push_back(p);
    }
    catch (boost::bad_lexical_cast &)
    {
        return false;
    }
    if (value_list.size() >= 2)
    {
        value.first = std::min(value_list[0], value_list[1]);
        value.second = std::max(value_list[0], value_list[1]);
        return true;
    }
    else
    {
        return false;
    }
}
