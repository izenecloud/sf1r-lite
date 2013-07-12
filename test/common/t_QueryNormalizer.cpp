/**
 * @file t_QueryNormalizer.cpp
 * @brief test normalizing the query string
 */

#include <common/QueryNormalizer.h>
#include <boost/test/unit_test.hpp>
#include <vector>
#include <iostream>

namespace
{

void checkNormalize(const std::string& source, const std::string& expect)
{
    std::string actual;
    sf1r::QueryNormalizer::get()->normalize(source, actual);

    BOOST_CHECK_EQUAL(actual, expect);
}

void checkCharNum(const std::string& source, std::size_t expect)
{
    std::size_t actual = sf1r::QueryNormalizer::get()->countCharNum(source);

    BOOST_TEST_MESSAGE("source: " << source <<
                       ", expect char num: " << expect <<
                       ", actual char num: " << actual);

    BOOST_CHECK_EQUAL(actual, expect);
}

void checkLongQuery(const std::string& source, bool expect)
{
    bool actual = sf1r::QueryNormalizer::get()->isLongQuery(source);

    BOOST_CHECK_EQUAL(actual, expect);
}

void checkProductType(const std::string& source, const std::vector<std::string>& expect)
{
    std::vector<std::string> actual;
    std::string newquery;
    sf1r::QueryNormalizer::get()->getProductTypes(source, actual, newquery);

    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(),
                             expect.begin(), expect.end());

    std::cout << "the new query is:" << newquery << std::endl << std::endl;
}

}

BOOST_AUTO_TEST_SUITE(QueryNormalizerTest)

BOOST_AUTO_TEST_CASE(testEmptyOutput)
{
    std::string source;
    const std::string empty;

    checkNormalize(source, empty);
    checkCharNum(source, 0);
    checkLongQuery(source, false);

    source = "  \t   \n";
    checkNormalize(source, empty);
    checkCharNum(source, 0);
    checkLongQuery(source, false);
}

BOOST_AUTO_TEST_CASE(testTokenizeByWhiteSpace)
{
    std::string source("aa bb cc dd ee");
    std::string expect(source);
    checkNormalize(source, expect);
    checkCharNum(source, 5);
    checkLongQuery(source, false);

    source = " aa  bb   cc\tdd\n\ree\n";
    checkNormalize(source, expect);
    checkCharNum(source, 5);
    checkLongQuery(source, false);
}

BOOST_AUTO_TEST_CASE(testConvertToLowerCase)
{
    checkNormalize("AAA", "aaa");
    checkNormalize("AA BB cc x Y Z", "aa bb cc x y z");
}

BOOST_AUTO_TEST_CASE(testSortByLexicalOrder)
{
    checkNormalize("k UU III", "iii k uu");
    checkNormalize("cc Z x AA BB Y", "aa bb cc x y z");

    std::string source("HTC one M7");
    checkNormalize(source, "htc m7 one");
    checkCharNum(source, 3);
    checkLongQuery(source, false);
}

BOOST_AUTO_TEST_CASE(testMixInput)
{
    std::string source("iPhone5手机");
    checkNormalize(source, "iphone5手机");
    checkCharNum(source, 3);
    checkLongQuery(source, false);

    source = "iPhone 5 手机";
    checkNormalize(source, "5 iphone 手机");
    checkCharNum(source, 4);
    checkLongQuery(source, false);

    source = "polo衫 短袖\t男\r\n";
    checkNormalize(source, "polo衫 男 短袖");
    checkCharNum(source, 5);
    checkLongQuery(source, false);

    source = "  三星  Galaxy  I9300  ";
    checkNormalize(source, "galaxy i9300 三星");
    checkCharNum(source, 4);
    checkLongQuery(source, false);

    source = "5.5寸手机 1.8 NO.1 P7";
    checkNormalize(source, "1.8 5.5寸手机 no.1 p7");
    checkCharNum(source, 7);
    checkLongQuery(source, false);

    source = "Haier/海尔 ES50H-D1(E) 50升";
    checkNormalize(source, "50升 es50h-d1(e) haier/海尔");
    checkCharNum(source, 8);
    checkLongQuery(source, true);

    source = "O.P.I 天然 -80# 珍珠色 L’OREAL/欧莱雅";
    checkNormalize(source, "-80# l’oreal/欧莱雅 o.p.i 天然 珍珠色");
    checkCharNum(source, 12);
    checkLongQuery(source, true);
}


BOOST_AUTO_TEST_CASE(testGetProductType)
{
    std::cout <<"Test :iphone5手机" << std::endl;
    std::string source_0("iphone5手机");
    std::vector<std::string> actual_0;
    actual_0.push_back("5");
    //actual_0.push_back("iphone");
    checkProductType(source_0, actual_0);
    
    std::cout <<"Test :皇冠Haier 海尔 KFR-35GW/05FFC23 1.5匹 挂壁式 无氟直流变频 冷暖空调" << std::endl;
    std::string source_1("皇冠Haier 海尔 KFR-35GW/05FFC23 1.5匹 挂壁式 无氟直流变频 冷暖空调");
    std::vector<std::string> actual_1;
    actual_1.push_back("05ffc23");
    actual_1.push_back("kfr-35gw");
    checkProductType(source_1, actual_1);

    std::cout <<"Test :全网底价联保 iphone 5 FOTILE 方太 CXW-189-EH16 【欧式吸油烟机】 + JZT-FZ26GE 嵌入式燃气灶（天然气）（套餐)送充电宝" << std::endl;
    std::string source_2("全网底价联保 iphone 5 FOTILE 方太 CXW-189-EH16 【欧式吸油烟机】 + JZT-FZ26GE 嵌入式燃气灶（天然气）（套餐)送充电宝");
    std::vector<std::string> actual_2;
    actual_2.push_back("5");
    actual_2.push_back("cxw-189-eh16");
    //actual_2.push_back("iphone");
    actual_2.push_back("jzt-fz26ge");
    checkProductType(source_2, actual_2);

    std::cout <<"Test :魔杰（Mogic)Q500P iphone 4/itouch5苹果 lightning接口专用 2.1音箱 卡通章鱼音箱白色" << std::endl;
    std::string source_3("魔杰（Mogic） q500p iphone 4/itouch5苹果 lightning接口专用 2.1音箱 卡通章鱼音箱白色");
    std::vector<std::string> actual_3;
    actual_3.push_back("4");
    actual_3.push_back("5");
    //actual_3.push_back("iphone");
    //actual_3.push_back("itouch");
    actual_3.push_back("q500p");
    checkProductType(source_3, actual_3);

    std::cout <<"Test :佰草集 清爽洁面乳 100ml 深层清洁 补水 保湿 洗面奶" << std::endl;
    std::string source_4("佰草集 清爽洁面乳 100ml 深层清洁 补水 保湿 洗面奶");
    std::vector<std::string> actual_4;
    actual_4.push_back("100ml");
    checkProductType(source_4, actual_4);

    std::cout <<"Test :ThinkPad E430(3254C34) 14英寸笔记本电脑 （i5-3230M 4G 500G 1G独显 蓝牙 WIN8）" << std::endl;
    std::string source_5("ThinkPad E430(3254C34) 14英寸笔记本电脑 （i5-3230M 4G 500G 1G独显 蓝牙 WIN8）");
    std::vector<std::string> actual_5;
    actual_5.push_back("e430");
    checkProductType(source_5, actual_5);


    std::cout <<"Test :Apple/苹果 iphone 4s无锁16G/32G现货促销全国包邮返100送充电宝" << std::endl;
    std::string source_6("Apple/苹果 iphone 4s无锁16G/32G现货促销全国包邮返100送充电宝");
    std::vector<std::string> actual_6;
    actual_6.push_back("4s");
    //actual_6.push_back("iphone");
    checkProductType(source_6, actual_6);


    std::cout <<"Test :送原电 送豪礼 Samsung/三星 GALAXY S4 I9500 I9505 盖世4(现货促销全国包)" << std::endl;
    std::string source_7("送原电 送豪礼 Samsung/三星 GALAXY S4 I9500 I9505 盖世4(现货促销全国包)");
    std::vector<std::string> actual_7;
    actual_7.push_back("i9500");
    actual_7.push_back("i9505");
    checkProductType(source_7, actual_7);

}

BOOST_AUTO_TEST_SUITE_END()
