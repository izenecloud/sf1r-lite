#ifndef SF1R_PRODUCTMANAGER_PRODUCTPRICE_H
#define SF1R_PRODUCTMANAGER_PRODUCTPRICE_H

#include <common/type_defs.h>

#include "pm_def.h"
#include "pm_types.h"


namespace sf1r
{

class ProductPrice : boost::addable<ProductPrice, boost::equality_comparable<ProductPrice> >
{
public:
    ProductPrice();

    ProductPrice(ProductPriceType a, ProductPriceType b);

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & value;
    }

    ProductPrice& operator+=(const ProductPrice& a);

    bool operator==(const ProductPrice& b) const;

    bool Parse(const izenelib::util::UString& ustr);

    bool Parse(const std::string& str);

    std::string ToString() const;

    Document::doc_prop_value_strtype ToPropString() const;

    bool Valid() const;

    bool Positive() const;

    bool GetMid(ProductPriceType& mid) const;

    ProductPriceType Min() const { return value.first; }
    ProductPriceType Max() const { return value.second; }
    ProductPriceType Mid() const { return (Min()+Max())/2.0; }
    friend std::ostream& operator<<(std::ostream& out, const ProductPrice& p)
    {
        if(p.Valid())
        {
            if(p.value.first!=p.value.second)
            {
                out<<p.value.first<<"-"<<p.value.second;
            }
            else
            {
                out<<p.value.first;
            }
        }
        return out;
    }

private:
    static bool Convert_(const std::string& str, ProductPriceType& p);
    static bool Convert_(ProductPriceType p, std::stringstream& ss);
    void Check_();

    void Reset_();

    bool checkSeparatorType_(const std::string& propertyValueStr, char separator);

    bool split_float_(const std::string& szText, char Separator);

public:
    std::pair<ProductPriceType, ProductPriceType> value;
};

}

#endif
