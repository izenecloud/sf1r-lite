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
    ProductPrice();

    ProductPrice(ProductPriceType a, ProductPriceType b);


    ProductPrice& operator+=(const ProductPrice& a);

    bool operator==(const ProductPrice& b) const;

    bool Parse(const izenelib::util::UString& ustr);

    bool Parse(const std::string& str);

    std::string ToString() const;

    izenelib::util::UString ToUString() const;

    bool Valid() const;

private:
    void Check_();

    void Reset_();

    bool checkSeparatorType_(const std::string& propertyValueStr, char separator);

    bool split_float_(const std::string& szText, char Separator );


public:
    std::pair<ProductPriceType, ProductPriceType> value;
};

}

#endif
