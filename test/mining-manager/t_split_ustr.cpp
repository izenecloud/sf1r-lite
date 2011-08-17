///
/// @file t_split_ustr.cpp
/// @brief test splitting UString into group/attribute values
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-08-17
///

#include <util/ustring/UString.h>
#include <mining-manager/util/split_ustr.h>

#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>

using namespace std;
using namespace boost;
using namespace sf1r;

namespace
{
const izenelib::util::UString::EncodingType ENCODING_TYPE = izenelib::util::UString::UTF_8;

void checkGroupValues(
    const vector<izenelib::util::UString>& groupValues,
    const vector<string>& goldGroupValues
)
{
    BOOST_CHECK_EQUAL(groupValues.size(), goldGroupValues.size());
    for (unsigned int i = 0; i < groupValues.size(); ++i)
    {
        std::string valueUtf8;
        groupValues[i].convertString(valueUtf8, ENCODING_TYPE);
        BOOST_CHECK_EQUAL(valueUtf8, goldGroupValues[i]);
    }
}

typedef pair<string, vector<string> > AttrPairStr;
void checkAttrPairs(
    const vector<AttrPair>& attrPairs,
    const vector<AttrPairStr>& goldAttrPairs
)
{
    BOOST_CHECK_EQUAL(attrPairs.size(), goldAttrPairs.size());
    for (unsigned int i = 0; i < attrPairs.size(); ++i)
    {
        std::string nameUtf8;
        attrPairs[i].first.convertString(nameUtf8, ENCODING_TYPE);
        BOOST_CHECK_EQUAL(nameUtf8, goldAttrPairs[i].first);

        BOOST_CHECK_EQUAL(attrPairs[i].second.size(), goldAttrPairs[i].second.size());
        for (unsigned int j = 0; j < attrPairs[i].second.size(); ++j)
        {
            std::string valueUtf8;
            attrPairs[i].second[j].convertString(valueUtf8, ENCODING_TYPE);
            BOOST_CHECK_EQUAL(valueUtf8, goldAttrPairs[i].second[j]);
        }
    }
}

}

BOOST_AUTO_TEST_SUITE(split_ustr_test)

BOOST_AUTO_TEST_CASE(split_group)
{
    {
        izenelib::util::UString src("", ENCODING_TYPE);
        vector<izenelib::util::UString> groupValues;
        split_group_value(src, groupValues);
        BOOST_CHECK_EQUAL(groupValues.size(), 0U);
    }

    {
        izenelib::util::UString src(",,,", ENCODING_TYPE);
        vector<izenelib::util::UString> groupValues;
        split_group_value(src, groupValues);
        BOOST_CHECK_EQUAL(groupValues.size(), 0U);
    }

    {
        izenelib::util::UString src("a,b,c", ENCODING_TYPE);
        vector<izenelib::util::UString> groupValues;
        split_group_value(src, groupValues);

        vector<string> goldGroupValues;
        goldGroupValues.push_back("a");
        goldGroupValues.push_back("b");
        goldGroupValues.push_back("c");

        checkGroupValues(groupValues, goldGroupValues);
    }

    {
        izenelib::util::UString src("T恤,衬衫, POLO衫,\"John, Mark \"\"Mary\"\"\",长袖上衣  ,,,,,", ENCODING_TYPE);
        vector<izenelib::util::UString> groupValues;
        split_group_value(src, groupValues);

        vector<string> goldGroupValues;
        goldGroupValues.push_back("T恤");
        goldGroupValues.push_back("衬衫");
        goldGroupValues.push_back("POLO衫");
        goldGroupValues.push_back("John, Mark \"Mary\"");
        goldGroupValues.push_back("长袖上衣");

        checkGroupValues(groupValues, goldGroupValues);
    }
}

BOOST_AUTO_TEST_CASE(split_attr)
{
    {
        izenelib::util::UString src(",,:,", ENCODING_TYPE);
        vector<AttrPair> attrPairs;
        split_attr_pair(src, attrPairs);
        BOOST_CHECK_EQUAL(attrPairs.size(), 0U);
    }

    {
        izenelib::util::UString src("abcde:", ENCODING_TYPE);
        vector<AttrPair> attrPairs;
        split_attr_pair(src, attrPairs);
        BOOST_CHECK_EQUAL(attrPairs.size(), 0U);
    }

    {
        izenelib::util::UString src("A:a,B:b,C:c", ENCODING_TYPE);
        vector<AttrPair> attrPairs;
        split_attr_pair(src, attrPairs);

        vector<AttrPairStr> goldAttrPairs;
        {
            AttrPairStr pairStr;
            pairStr.first = "A";
            pairStr.second.push_back("a");
            goldAttrPairs.push_back(pairStr);
        }
        {
            AttrPairStr pairStr;
            pairStr.first = "B";
            pairStr.second.push_back("b");
            goldAttrPairs.push_back(pairStr);
        }
        {
            AttrPairStr pairStr;
            pairStr.first = "C";
            pairStr.second.push_back("c");
            goldAttrPairs.push_back(pairStr);
        }

        checkAttrPairs(attrPairs, goldAttrPairs);
    }

    {
        izenelib::util::UString src("品牌: Two In One/欧艾尼 ,英文名:\"John, \nMark: \"\"Mary\"\" | Tom\",领子:圆领,季节:春季|\"夏|季\" abc |秋季,年份:2011,尺码:S|\" M \"| L|XL  ,,,:,,", ENCODING_TYPE);
        vector<AttrPair> attrPairs;
        split_attr_pair(src, attrPairs);

        vector<AttrPairStr> goldAttrPairs;
        {
            AttrPairStr pairStr;
            pairStr.first = "品牌";
            pairStr.second.push_back("Two In One/欧艾尼");
            goldAttrPairs.push_back(pairStr);
        }
        {
            AttrPairStr pairStr;
            pairStr.first = "英文名";
            pairStr.second.push_back("John, \nMark: \"Mary\" | Tom");
            goldAttrPairs.push_back(pairStr);
        }
        {
            AttrPairStr pairStr;
            pairStr.first = "领子";
            pairStr.second.push_back("圆领");
            goldAttrPairs.push_back(pairStr);
        }
        {
            AttrPairStr pairStr;
            pairStr.first = "季节";
            pairStr.second.push_back("春季");
            pairStr.second.push_back("夏|季");
            pairStr.second.push_back("秋季");
            goldAttrPairs.push_back(pairStr);
        }
        {
            AttrPairStr pairStr;
            pairStr.first = "年份";
            pairStr.second.push_back("2011");
            goldAttrPairs.push_back(pairStr);
        }
        {
            AttrPairStr pairStr;
            pairStr.first = "尺码";
            pairStr.second.push_back("S");
            pairStr.second.push_back(" M ");
            pairStr.second.push_back("L");
            pairStr.second.push_back("XL");
            goldAttrPairs.push_back(pairStr);
        }

        checkAttrPairs(attrPairs, goldAttrPairs);
    }
}

BOOST_AUTO_TEST_SUITE_END() 
