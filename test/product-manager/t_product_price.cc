#include <product-manager/product_price.h>
#include <boost/test/unit_test.hpp>

#include <boost/lexical_cast.hpp>

using namespace sf1r;

BOOST_AUTO_TEST_SUITE(ProductPrice_test)

BOOST_AUTO_TEST_CASE(parse_test)
{
    ProductPrice price;
    price.Parse("4.0-5.0");
    BOOST_CHECK(price.value.first==4.0);
    BOOST_CHECK(price.value.second==5.0);
    
    price.Parse("");
    BOOST_CHECK(price.Valid()==false);
    price.Parse("  ");
    BOOST_CHECK(price.Valid()==false);
    price.Parse("asd");
    BOOST_CHECK(price.Valid()==false);
    price.Parse("5.0");
    BOOST_CHECK(price.value.first==5.0);
    BOOST_CHECK(price.value.second==5.0);
}

BOOST_AUTO_TEST_CASE(add_test)
{
    ProductPrice price1, price2;
    price1.Parse("4.0-5.0");
    price2.Parse("");
    price1 += price2;
    BOOST_CHECK(price1.Valid()==true);
    BOOST_CHECK(price1.value.first==4.0);
    BOOST_CHECK(price1.value.second==5.0);
    price2.Parse("7.0");
    price1 += price2;
    BOOST_CHECK(price1.Valid()==true);
    BOOST_CHECK(price1.value.first==4.0);
    BOOST_CHECK(price1.value.second==7.0);

}

BOOST_AUTO_TEST_CASE(equal_test)
{
    ProductPrice price1, price2;
    price1.Parse("4.0-5.0");
    price2.Parse("");
    ProductPrice price3 = price1 + price2;
    BOOST_CHECK(price1==price3);

}

BOOST_AUTO_TEST_SUITE_END() 
