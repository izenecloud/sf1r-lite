/**
 * @file t_ScdParser.cpp
 * @author Ian Yang
 * @date Created <2010-08-25 14:28:52>
 *
 * @date Updated (Jun Jiang) <2011-05-19>
 * @brief add case "testUserId" and "testItemId" testing SCD files for recommend manager.
 */
#include <boost/test/unit_test.hpp>
#include <common/ScdParser.h>

#include <iostream>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/lexical_cast.hpp>

namespace fs = boost::filesystem;

namespace { // {anonymous}
class ScdBuilder
{
public:
    ScdBuilder(const fs::path& filePath, bool appendTrailingNewline = true)
    : appendTrailingNewline_(appendTrailingNewline)
    {
        out_.open(filePath, std::ios_base::out);
    }

    ~ScdBuilder()
    {
        if (appendTrailingNewline_)
        {
            out_ << std::endl;
        }
        out_.close();
    }

    std::size_t written()
    {
        return out_.tellp();
    }

    ScdBuilder& operator()(const std::string propertyName)
    {
        if (out_.tellp() != std::streampos(0))
        {
            out_ << std::endl;
        }

        out_ << "<" << propertyName << ">";
        return *this;
    }

    template<typename T>
    ScdBuilder& operator<<(const T& content)
    {
        out_ << content;
        return *this;
    }

private:
    bool appendTrailingNewline_;
    fs::ofstream out_;
};

class ScdParserFixture
{
public:
    ScdParserFixture()
    : path("scd/index/B-00-200912311000-11111-I-C.SCD"),
      fileName("B-00-200912311000-11111-I-C.SCD")
    {}

    fs::path createCleanTempDir(const std::string& name)
    {
        fs::path tmpdir = fs::path("tmp") / "ScdParser" / name;
        remove_all(tmpdir);
        create_directories(tmpdir);

        return tmpdir;
    }

    std::string path;
    std::string fileName;
    ScdParser parser;
}; // class ScdParserFixture
} // namespace {anonymous}

BOOST_FIXTURE_TEST_SUITE(ScdParser_test, ScdParserFixture)

BOOST_AUTO_TEST_CASE(testCheckSCDDate)
{
    BOOST_CHECK(ScdParser::checkSCDDate(path) == 20091231U);
    BOOST_CHECK(ScdParser::checkSCDDate(fileName) == 20091231U);
    BOOST_CHECK(ScdParser::checkSCDDate("") == 0U);
}

BOOST_AUTO_TEST_CASE(testCheckSCDTime)
{
    BOOST_CHECK(ScdParser::checkSCDTime(path) == 100011111U);
    BOOST_CHECK(ScdParser::checkSCDTime(fileName) == 100011111U);
    BOOST_CHECK(ScdParser::checkSCDTime("") == 0U);
}

BOOST_AUTO_TEST_CASE(testCheckSCDType)
{
    BOOST_CHECK(ScdParser::checkSCDType(path) == INSERT_SCD);
    BOOST_CHECK(ScdParser::checkSCDType(fileName) == INSERT_SCD);
    BOOST_CHECK(ScdParser::checkSCDType("") == NOT_SCD);

    path = "scd/index/B-00-200912311000-11111-i-C.SCD";
    BOOST_CHECK(ScdParser::checkSCDType(path) == INSERT_SCD);

    path = "scd/index/B-00-200912311000-11111-U-C.SCD";
    BOOST_CHECK(ScdParser::checkSCDType(path) == UPDATE_SCD);

    path = "scd/index/B-00-200912311000-11111-u-C.SCD";
    BOOST_CHECK(ScdParser::checkSCDType(path) == UPDATE_SCD);

    path = "scd/index/B-00-200912311000-11111-D-C.SCD";
    BOOST_CHECK(ScdParser::checkSCDType(path) == DELETE_SCD);

    path = "scd/index/B-00-200912311000-11111-d-C.SCD";
    BOOST_CHECK(ScdParser::checkSCDType(path) == DELETE_SCD);

    path = "scd/index/B-00-200912311000-11111-X-C.SCD";
    BOOST_CHECK(ScdParser::checkSCDType(path) == NOT_SCD);

    path = "scd/index/B-00-200912311000-11111-x-C.SCD";
    BOOST_CHECK(ScdParser::checkSCDType(path) == NOT_SCD);
}

BOOST_AUTO_TEST_CASE(testCheckSCDFormat)
{
    BOOST_CHECK(ScdParser::checkSCDFormat(path));
    BOOST_CHECK(ScdParser::checkSCDFormat(fileName));

    // empty
    BOOST_CHECK(!ScdParser::checkSCDFormat(""));

    // not SCD
    path = "scd/index/B-00-200912311000-11111-I-C.SXD";
    BOOST_CHECK(!ScdParser::checkSCDFormat(path));

    // not start with B
    path = "scd/index/C-00-200912311000-11111-U-C.SCD";
    BOOST_CHECK(!ScdParser::checkSCDFormat(path));

    // invalid year
    path = "scd/index/B-00-196912311000-11111-U-C.SCD";
    BOOST_CHECK(!ScdParser::checkSCDFormat(path));

    // invalid month
    path = "scd/index/B-00-200000311000-11111-U-C.SCD";
    BOOST_CHECK(!ScdParser::checkSCDFormat(path));
    path = "scd/index/B-00-200013311000-11111-U-C.SCD";
    BOOST_CHECK(!ScdParser::checkSCDFormat(path));

    // invalid day
    path = "scd/index/B-00-200012000000-11111-U-C.SCD";
    BOOST_CHECK(!ScdParser::checkSCDFormat(path));
    path = "scd/index/B-00-200012320000-11111-U-C.SCD";
    BOOST_CHECK(!ScdParser::checkSCDFormat(path));

    // invalid hour
    path = "scd/index/B-00-200012012500-11111-U-C.SCD";
    BOOST_CHECK(!ScdParser::checkSCDFormat(path));

    // invalid minutes
    path = "scd/index/B-00-200012010060-11111-U-C.SCD";
    BOOST_CHECK(!ScdParser::checkSCDFormat(path));

    // invalid minutes
    path = "scd/index/B-00-200012010000-60111-U-C.SCD";
    BOOST_CHECK(!ScdParser::checkSCDFormat(path));

    // invalid type
    path = "scd/index/B-00-200012010000-11111-X-C.SCD";
    BOOST_CHECK(!ScdParser::checkSCDFormat(path));

    // last byte is not c or f
    path = "scd/index/B-00-200012010000-11111-D-X.SCD";
    BOOST_CHECK(!ScdParser::checkSCDFormat(path));
}

BOOST_AUTO_TEST_CASE(testGetFileSize)
{
    fs::path tmpdir = createCleanTempDir("testGetFileSize");
    fs::path scdPath = tmpdir / fileName;

    long expected = 0;
    {
        ScdBuilder scd(scdPath);
        for (int i = 0; i < 10; ++i)
        {
            scd("DOCID") << i;
            scd("Title") << "Title " << i;
            scd("Content") << "Content " << i;
        }
        expected = scd.written() + 1; // count last new line
    }

    parser.load(scdPath.file_string());
    BOOST_CHECK_EQUAL(expected, parser.getFileSize());
}

BOOST_AUTO_TEST_CASE(testCannotLoadNonExistFile)
{
    fs::path tmpdir = createCleanTempDir("testCannotLoadNonExistFile");
    fs::path scdPath = tmpdir / fileName;

    BOOST_CHECK(!parser.load(scdPath.file_string()));
}

BOOST_AUTO_TEST_CASE(testGetDocIdList)
{
    fs::path tmpdir = createCleanTempDir("testGetDocIdList");
    fs::path scdPath = tmpdir / fileName;

    {
        ScdBuilder scd(scdPath);
        for (int i = 1; i <= 10; ++i)
        {
            scd("DOCID") << i;
            scd("Title") << "Title " << i;
            scd("Content") << "Content " << i;
        }
    }

    BOOST_CHECK(parser.load(scdPath.file_string()));

    std::vector<izenelib::util::UString> idList;
    BOOST_CHECK(parser.getDocIdList(idList));
    BOOST_CHECK(idList.size() == 10);

    izenelib::util::UString one("1", izenelib::util::UString::UTF_8);
    izenelib::util::UString ten("10", izenelib::util::UString::UTF_8);

    BOOST_CHECK(idList.front() == one);
    BOOST_CHECK(idList.back() == ten);
}

BOOST_AUTO_TEST_CASE(testIterator)
{
    fs::path tmpdir = createCleanTempDir("testIterator");
    fs::path scdPath = tmpdir / fileName;

    const int DOC_NUM = 3;
    {
        ScdBuilder scd(scdPath);
        for (int i = 1; i <= DOC_NUM; ++i)
        {
            scd("DOCID") << i;
            scd("Title") << "Title " << i;
            scd("Content") << "Content " << i;
        }
    }

    BOOST_CHECK(parser.load(scdPath.file_string()));

    izenelib::util::UString docid("DOCID", izenelib::util::UString::UTF_8);
    izenelib::util::UString title("Title", izenelib::util::UString::UTF_8);
    izenelib::util::UString content("Content", izenelib::util::UString::UTF_8);

    int docNum = 0;
    for (ScdParser::iterator doc_iter = parser.begin(); doc_iter != parser.end(); ++doc_iter)
    {
    SCDDocPtr doc = (*doc_iter);

    BOOST_CHECK_EQUAL(doc->size(), 3);

    std::string idStr = boost::lexical_cast<std::string>(docNum + 1);
    izenelib::util::UString idUStr(idStr, izenelib::util::UString::UTF_8);
    izenelib::util::UString titleUStr("Title " + idStr, izenelib::util::UString::UTF_8);
    izenelib::util::UString contentUStr("Content " + idStr, izenelib::util::UString::UTF_8);

    // <DOCID>
    BOOST_CHECK((*doc)[0].first == docid);
    BOOST_CHECK_EQUAL(idUStr, (*doc)[0].second);

    // <Title>
    BOOST_CHECK((*doc)[1].first == title);
    BOOST_CHECK_EQUAL(titleUStr, (*doc)[1].second);

    // <Content>
    BOOST_CHECK((*doc)[2].first == content);
    BOOST_CHECK_EQUAL(contentUStr, (*doc)[2].second);

    ++docNum;
    }

    BOOST_CHECK_EQUAL(docNum, DOC_NUM);
}

BOOST_AUTO_TEST_CASE(testEmptySCD)
{
    fs::path tmpdir = createCleanTempDir("testEmptySCD");
    fs::path scdPath = tmpdir / fileName;

    {
        ScdBuilder scd(scdPath);
    }

    BOOST_CHECK(parser.load(scdPath.file_string()));

    std::vector<izenelib::util::UString> idList;
    BOOST_CHECK(parser.getDocIdList(idList));


    BOOST_CHECK(idList.size() == 0);

    int docNum = 0;
    for (ScdParser::iterator doc_iter = parser.begin(); doc_iter != parser.end(); ++doc_iter)
    {
        ++docNum;
    }

    BOOST_CHECK_EQUAL(docNum, 0);
}

BOOST_AUTO_TEST_CASE(testOnlyOneDOCID)
{
    fs::path tmpdir = createCleanTempDir("testOnlyOneDOCID");
    fs::path scdPath = tmpdir / fileName;

    {
        ScdBuilder scd(scdPath);
        scd("DOCID") << 1;
    }

    BOOST_CHECK(parser.load(scdPath.file_string()));

    std::vector<izenelib::util::UString> idList;
    BOOST_CHECK(parser.getDocIdList(idList));

    izenelib::util::UString docid("DOCID", izenelib::util::UString::UTF_8);
    izenelib::util::UString one("1", izenelib::util::UString::UTF_8);

    BOOST_CHECK(idList.size() == 1);
    BOOST_CHECK_EQUAL(one, idList.front());

    int docNum = 0;
    for (ScdParser::iterator doc_iter = parser.begin(); doc_iter != parser.end(); ++doc_iter)
    {
    SCDDocPtr doc = (*doc_iter);

    BOOST_CHECK(doc->size() == 1);

    // <DOCID>
    BOOST_CHECK((*doc)[0].first == docid);
    BOOST_CHECK_EQUAL(one, (*doc)[0].second);

    ++docNum;
    }

    BOOST_CHECK_EQUAL(docNum, 1);
}

BOOST_AUTO_TEST_CASE(testOnlyOneDoc)
{
    fs::path tmpdir = createCleanTempDir("testOnlyOneDoc");
    fs::path scdPath = tmpdir / fileName;

    {
        ScdBuilder scd(scdPath);
        scd("DOCID") << 1;
        scd("Title") << "Title " << 1;
        scd("Content") << "Content " << 1;
    }

    BOOST_CHECK(parser.load(scdPath.file_string()));

    std::vector<izenelib::util::UString> idList;
    BOOST_CHECK(parser.getDocIdList(idList));

    izenelib::util::UString docid("DOCID", izenelib::util::UString::UTF_8);
    izenelib::util::UString title("Title", izenelib::util::UString::UTF_8);
    izenelib::util::UString content("Content", izenelib::util::UString::UTF_8);

    izenelib::util::UString one("1", izenelib::util::UString::UTF_8);
    izenelib::util::UString titleOne("Title 1", izenelib::util::UString::UTF_8);
    izenelib::util::UString contentOne("Content 1", izenelib::util::UString::UTF_8);

    BOOST_CHECK(idList.size() == 1);
    BOOST_CHECK_EQUAL(one, idList.front());

    int docNum = 0;
    for (ScdParser::iterator doc_iter = parser.begin(); doc_iter != parser.end(); ++doc_iter)
    {
    SCDDocPtr doc = (*doc_iter);

    BOOST_CHECK(doc->size() == 3);

    // <DOCID>
    BOOST_CHECK((*doc)[0].first == docid);
    BOOST_CHECK_EQUAL(one, (*doc)[0].second);

    // <Title>
    BOOST_CHECK((*doc)[1].first == title);
    BOOST_CHECK_EQUAL(titleOne, (*doc)[1].second);

    // <Content>
    BOOST_CHECK((*doc)[2].first == content);
    BOOST_CHECK_EQUAL(contentOne, (*doc)[2].second);

    ++docNum;
    }

    BOOST_CHECK_EQUAL(docNum, 1);
}

BOOST_AUTO_TEST_CASE(testNoTrailingNewLine)
{
    fs::path tmpdir = createCleanTempDir("testNoTrailingNewLine");
    fs::path scdPath = tmpdir / fileName;

    {
        ScdBuilder scd(scdPath, false);
        scd("DOCID") << 1;
        scd("Title") << "Title " << 1;
        scd("Content") << "Content " << 1;
    }

    BOOST_CHECK(parser.load(scdPath.file_string()));


    izenelib::util::UString docid("DOCID", izenelib::util::UString::UTF_8);
    izenelib::util::UString title("Title", izenelib::util::UString::UTF_8);
    izenelib::util::UString content("Content", izenelib::util::UString::UTF_8);

    izenelib::util::UString one("1", izenelib::util::UString::UTF_8);
    izenelib::util::UString titleOne("Title 1", izenelib::util::UString::UTF_8);
    izenelib::util::UString contentOne("Content 1", izenelib::util::UString::UTF_8);


    for (ScdParser::iterator doc_iter = parser.begin(); doc_iter != parser.end(); ++doc_iter)
    {
    SCDDocPtr doc = (*doc_iter);

    BOOST_CHECK(doc->size() == 3);

    // <DOCID>
    BOOST_CHECK((*doc)[0].first == docid);
    BOOST_CHECK_EQUAL(one, (*doc)[0].second);

    // <Title>
    BOOST_CHECK((*doc)[1].first == title);
    BOOST_CHECK_EQUAL(titleOne, (*doc)[1].second);

    // <Content>
    BOOST_CHECK((*doc)[2].first == content);
    BOOST_CHECK_EQUAL(contentOne, (*doc)[2].second);
    }
}

BOOST_AUTO_TEST_CASE(testTrailingCarriageReturn)
{
    fs::path tmpdir = createCleanTempDir("testTrailingCarriageReturn");
    fs::path scdPath = tmpdir / fileName;

    const int DOC_NUM = 3;
    {
        ScdBuilder scd(scdPath);
        for (int i = 1; i <= DOC_NUM; ++i)
        {
            scd("DOCID") << i << "\r";
            scd("Title") << "Title " << i << "\r";
            scd("Content") << "Content " << i << "\r";
        }
    }

    BOOST_CHECK(parser.load(scdPath.file_string()));

    izenelib::util::UString docid("DOCID", izenelib::util::UString::UTF_8);
    izenelib::util::UString title("Title", izenelib::util::UString::UTF_8);
    izenelib::util::UString content("Content", izenelib::util::UString::UTF_8);

    int docNum = 0;
    for (ScdParser::iterator doc_iter = parser.begin(); doc_iter != parser.end(); ++doc_iter)
    {
        SCDDocPtr doc = (*doc_iter);

        BOOST_CHECK_EQUAL(doc->size(), 3);

        std::string idStr = boost::lexical_cast<std::string>(docNum + 1);
        izenelib::util::UString idUStr(idStr, izenelib::util::UString::UTF_8);
        izenelib::util::UString titleUStr("Title " + idStr, izenelib::util::UString::UTF_8);
        izenelib::util::UString contentUStr("Content " + idStr, izenelib::util::UString::UTF_8);

        // <DOCID>
        BOOST_CHECK((*doc)[0].first == docid);
        BOOST_CHECK_EQUAL(idUStr, (*doc)[0].second);

        // <Title>
        BOOST_CHECK((*doc)[1].first == title);
        BOOST_CHECK_EQUAL(titleUStr, (*doc)[1].second);

        // <Content>
        BOOST_CHECK((*doc)[2].first == content);
        BOOST_CHECK_EQUAL(contentUStr, (*doc)[2].second);

        ++docNum;
    }
}

BOOST_AUTO_TEST_CASE(testUserId)
{
    fs::path tmpdir = createCleanTempDir("testUserId");
    fs::path scdPath = tmpdir / fileName;

    const int DOC_NUM = 5;
    {
        ScdBuilder scd(scdPath);
        for (int i = 1; i <= DOC_NUM; ++i)
        {
            scd("USERID") << "user_" << i;
            scd("gender") << "gender_" << i;
            scd("age") << "age_" << i;
            scd("area") << "area_" << i;
        }
    }

    ScdParser userParser(izenelib::util::UString::UTF_8, "<USERID>");
    BOOST_CHECK(userParser.load(scdPath.file_string()));

    izenelib::util::UString userid("USERID", izenelib::util::UString::UTF_8);
    izenelib::util::UString gender("gender", izenelib::util::UString::UTF_8);
    izenelib::util::UString age("age", izenelib::util::UString::UTF_8);
    izenelib::util::UString area("area", izenelib::util::UString::UTF_8);

    int docNum = 0;
    for (ScdParser::iterator doc_iter = userParser.begin(); doc_iter != userParser.end(); ++doc_iter)
    {
    SCDDocPtr doc = (*doc_iter);

    BOOST_CHECK(doc->size() == 4);

    std::string idStr = boost::lexical_cast<std::string>(docNum + 1);
    izenelib::util::UString idUStr("user_" + idStr, izenelib::util::UString::UTF_8);
    izenelib::util::UString genderUStr("gender_" + idStr, izenelib::util::UString::UTF_8);
    izenelib::util::UString ageUStr("age_" + idStr, izenelib::util::UString::UTF_8);
    izenelib::util::UString areaUStr("area_" + idStr, izenelib::util::UString::UTF_8);

    // <UESRID>
    BOOST_CHECK((*doc)[0].first == userid);
    BOOST_CHECK_EQUAL(idUStr, (*doc)[0].second);

    // <gender>
    BOOST_CHECK((*doc)[1].first == gender);
    BOOST_CHECK_EQUAL(genderUStr, (*doc)[1].second);

    // <age>
    BOOST_CHECK((*doc)[2].first == age);
    BOOST_CHECK_EQUAL(ageUStr, (*doc)[2].second);

    // <area>
    BOOST_CHECK((*doc)[3].first == area);
    BOOST_CHECK_EQUAL(areaUStr, (*doc)[3].second);

    ++docNum;
    }

    BOOST_CHECK_EQUAL(docNum, DOC_NUM);
}

BOOST_AUTO_TEST_CASE(testItemId)
{
    fs::path tmpdir = createCleanTempDir("testItemId");
    fs::path scdPath = tmpdir / fileName;

    const int DOC_NUM = 10;
    {
        ScdBuilder scd(scdPath);
        for (int i = 1; i <= DOC_NUM; ++i)
        {
            scd("ITEMID") << "item_" << i;
            scd("name") << "名字_" << i;
            scd("price") << "price_" << i;
            scd("link") << "www.shop.com/product/item_" << i;
            scd("category") << "分类_" << i;
        }
    }

    ScdParser itemParser(izenelib::util::UString::UTF_8, "<ITEMID>");
    BOOST_CHECK(itemParser.load(scdPath.file_string()));

    izenelib::util::UString userid("ITEMID", izenelib::util::UString::UTF_8);
    izenelib::util::UString name("name", izenelib::util::UString::UTF_8);
    izenelib::util::UString price("price", izenelib::util::UString::UTF_8);
    izenelib::util::UString link("link", izenelib::util::UString::UTF_8);
    izenelib::util::UString category("category", izenelib::util::UString::UTF_8);

    int docNum = 0;
    for (ScdParser::iterator doc_iter = itemParser.begin(); doc_iter != itemParser.end(); ++doc_iter)
    {
    SCDDocPtr doc = (*doc_iter);

    BOOST_CHECK(doc->size() == 5);

    std::string idStr = boost::lexical_cast<std::string>(docNum + 1);
    izenelib::util::UString idUStr("item_" + idStr, izenelib::util::UString::UTF_8);
    izenelib::util::UString nameUStr("名字_" + idStr, izenelib::util::UString::UTF_8);
    izenelib::util::UString priceUStr("price_" + idStr, izenelib::util::UString::UTF_8);
    izenelib::util::UString linkUStr("www.shop.com/product/item_" + idStr, izenelib::util::UString::UTF_8);
    izenelib::util::UString categoryUStr("分类_" + idStr, izenelib::util::UString::UTF_8);

    // <UESRID>
    BOOST_CHECK((*doc)[0].first == userid);
    BOOST_CHECK_EQUAL(idUStr, (*doc)[0].second);

    // <name>
    BOOST_CHECK((*doc)[1].first == name);
    BOOST_CHECK_EQUAL(nameUStr, (*doc)[1].second);

    // <price>
    BOOST_CHECK((*doc)[2].first == price);
    BOOST_CHECK_EQUAL(priceUStr, (*doc)[2].second);

    // <link>
    BOOST_CHECK((*doc)[3].first == link);
    BOOST_CHECK_EQUAL(linkUStr, (*doc)[3].second);

    // <category>
    BOOST_CHECK((*doc)[4].first == category);
    BOOST_CHECK_EQUAL(categoryUStr, (*doc)[4].second);

    ++docNum;
    }

    BOOST_CHECK_EQUAL(docNum, DOC_NUM);
}

BOOST_AUTO_TEST_SUITE_END() // ScdParser_test
