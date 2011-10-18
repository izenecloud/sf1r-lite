#ifndef SF1R_PRODUCTMANAGER_PRODUCTPRICE_H
#define SF1R_PRODUCTMANAGER_PRODUCTPRICE_H

#include <sstream>
#include <common/type_defs.h>

#include <boost/operators.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include "pm_def.h"
#include "pm_types.h"


namespace sf1r
{

class ProductPrice : boost::addable< ProductPrice, boost::equality_comparable<ProductPrice> >
{
public:
    typedef double ProductPriceType;
    
    ProductPrice():value(0.0,0.0)
    {
    }
    
    ProductPrice(ProductPriceType a, ProductPriceType b): value(a,b)
    {
        Check_();
    }
    
    
    ProductPrice& operator+=(const ProductPrice& a)
    {
        value.first = std::min(value.first, a.value.first);
        value.second = std::max(value.second, a.value.second);
        return *this;
    }
    
    bool operator==(const ProductPrice& b) const
    {
        return (value.first==b.value.first&&value.second==b.value.second);
    }
    
    bool Parse(const izenelib::util::UString& ustr)
    {
        try
        {
            std::string str;
            ustr.convertString(str, izenelib::util::UString::UTF_8);
            ProductPriceType p = boost::lexical_cast<ProductPriceType>(str);
            value.first = p;
            value.second = p;
        }
        catch( const boost::bad_lexical_cast & )
        {
            char sep[] = {'-', '~', ','};
            uint32_t len = sizeof(sep)/sizeof(char);
            bool found = false;
            for(uint32_t i=0;i<len;i++)
            {
                if(checkSeparatorType_(ustr, sep[i]) )
                {
                    found = true;
                    if(!split_float_(ustr, sep[i]))
                    {
                        Reset_();
                        return false;
                    }
                    break;
                }
            }
            if(!found) 
            {
                Reset_();
                return false;
            }
        }
        return true;
    }
    
    std::string ToString() const
    {
        std::stringstream ss;
        if(value.first==value.second)
        {
            ss<<value.first;
        }
        else
        {
            ss<<value.first<<"-"<<value.second;
        }
        return ss.str();
    }
    
    izenelib::util::UString ToUString() const
    {
        return izenelib::util::UString(ToString(), izenelib::util::UString::UTF_8);
    }

    
    std::pair<ProductPriceType, ProductPriceType> value;
    
    
private:
    void Check_()
    {
        if(value.first>value.second)
        {
            //invalid
            Reset_();
        }
    }
    
    void Reset_()
    {
        value.first = 0.0;
        value.second = 0.0;
    }
    
    bool checkSeparatorType_(const izenelib::util::UString& propertyValueStr, char separator)
    {
        izenelib::util::UString tmpStr(propertyValueStr);
        izenelib::util::UString sep(" ",izenelib::util::UString::UTF_8);
        sep[0] = separator;
        size_t n = 0;
        n = tmpStr.find(sep,0);
        if (n != izenelib::util::UString::npos)
            return true;
        return false;
    }
    
    bool split_float_(const izenelib::util::UString& szText, char Separator )
    {
        izenelib::util::UString str(szText);
        izenelib::util::UString sep(" ",izenelib::util::UString::UTF_8);
        sep[0] = Separator;
        std::size_t n = 0, nOld=0;
        std::vector<ProductPriceType> value_list;
        while (n != izenelib::util::UString::npos)
        {
            n = str.find(sep,n);
            if (n != izenelib::util::UString::npos)
            {
                if (n != nOld)
                {
                    try
                    {
                        izenelib::util::UString tmpStr = str.substr(nOld, n-nOld);
                        boost::algorithm::trim( tmpStr );
                        ProductPriceType p = boost::lexical_cast< ProductPriceType >( tmpStr );
                        value_list.push_back(p);
                    }
                    catch( const boost::bad_lexical_cast & )
                    {
                        return false;
                    }
                }
                n += sep.length();
                nOld = n;
            }
        }

        try
        {
            izenelib::util::UString tmpStr = str.substr(nOld, str.length()-nOld);
            trim( tmpStr );
            ProductPriceType p = boost::lexical_cast< ProductPriceType >( tmpStr );
            value_list.push_back(p);
        }
        catch( const boost::bad_lexical_cast & )
        {
            return false;
        }
        if(value_list.size()>=2)
        {
            value.first = value_list[0];
            value.second = value_list[1];
            return true;
        }
        else
        {
            return false;
        }
    }
    
};

}

#endif

