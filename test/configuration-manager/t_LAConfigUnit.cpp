/**
 * @file t_LAConfigUnit.cpp
 * @author Ian Yang
 * @date Created <2010-08-25 10:54:25>
 * @brief 
 */
#include <boost/test/unit_test.hpp>

#include <configuration-manager/LAConfigUnit.h>

using namespace sf1r;

namespace { // {anonymous}
class LAConfigUnitFixture
{
public:
    LAConfigUnit subject;
}; // class LAConfigUnitFixture

} // namespace {anonymous}

BOOST_FIXTURE_TEST_SUITE(LAConfigUnit_test, LAConfigUnitFixture)

BOOST_AUTO_TEST_CASE(testId)
{
    subject.setId("id");
    BOOST_CHECK(subject.getId() == "id");
}

BOOST_AUTO_TEST_CASE(testAnalysis)
{
    subject.setAnalysis("analysis");
    BOOST_CHECK(subject.getAnalysis() == "analysis");
}

BOOST_AUTO_TEST_CASE(testReferenceMethod)
{
    LAConfigUnit referred;
    referred.setId("referred");
    referred.setLanguage("CN");

    subject.addReferenceMethod(referred);
    std::string methodId;
    BOOST_CHECK(subject.getMethodIdByLang("CN", methodId));
    BOOST_CHECK(methodId == "referred");
}

BOOST_AUTO_TEST_CASE(testLevel)
{
    subject.setLevel(10);
    BOOST_CHECK(subject.getLevel() == 10);

    subject.setMinLevel(1);
    BOOST_CHECK(subject.getMinLevel() == 1);

    subject.setMaxLevel(100);
    BOOST_CHECK(subject.getMaxLevel() == 100);
}

BOOST_AUTO_TEST_CASE(testApart)
{
    subject.setApart(true);
    BOOST_CHECK(subject.getApart());

    subject.setApart(false);
    BOOST_CHECK(!subject.getApart());
}

BOOST_AUTO_TEST_CASE(testMaxNo)
{
    subject.setMaxNo(10);
    BOOST_CHECK(subject.getMaxNo() == 10);
}

BOOST_AUTO_TEST_CASE(testPrefix)
{
    subject.setPrefix(true);
    BOOST_CHECK(subject.getPrefix());

    subject.setPrefix(false);
    BOOST_CHECK(!subject.getPrefix());
}

BOOST_AUTO_TEST_CASE(testSuffix)
{
    subject.setSuffix(true);
    BOOST_CHECK(subject.getSuffix());

    subject.setSuffix(false);
    BOOST_CHECK(!subject.getSuffix());
}

BOOST_AUTO_TEST_CASE(testLanguage)
{
    subject.setLanguage("language");
    BOOST_CHECK(subject.getLanguage() == "language");
}

BOOST_AUTO_TEST_CASE(testMode)
{
    subject.setMode("mode");
    BOOST_CHECK(subject.getMode() == "mode");
}

BOOST_AUTO_TEST_CASE(testOption)
{
    subject.setOption("option");
    BOOST_CHECK(subject.getOption() == "option");
}

BOOST_AUTO_TEST_CASE(testSpecialChar)
{
    subject.setSpecialChar("SpecialChar");
    BOOST_CHECK(subject.getSpecialChar() == "SpecialChar");
}

BOOST_AUTO_TEST_CASE(testDictionaryPath)
{
    subject.setDictionaryPath("DictionaryPath");
    BOOST_CHECK(subject.getDictionaryPath() == "DictionaryPath");
}

BOOST_AUTO_TEST_CASE(testCaseSensitive)
{
    subject.setCaseSensitive(true);
    BOOST_CHECK(subject.getCaseSensitive());

    subject.setCaseSensitive(false);
    BOOST_CHECK(!subject.getCaseSensitive());
}

BOOST_AUTO_TEST_CASE(testAdvOption)
{
    subject.setAdvOption("AdvOption");
    BOOST_CHECK(subject.getAdvOption() == "AdvOption");
}

BOOST_AUTO_TEST_CASE(testIdxFlag)
{
    subject.setIdxFlag('a');
    BOOST_CHECK(subject.getIdxFlag() == 'a');
}

BOOST_AUTO_TEST_CASE(testSchFlag)
{
    subject.setSchFlag('a');
    BOOST_CHECK(subject.getSchFlag() == 'a');
}

BOOST_AUTO_TEST_CASE(testStem)
{
    subject.setStem(true);
    BOOST_CHECK(subject.getStem());

    subject.setStem(false);
    BOOST_CHECK(!subject.getStem());
}

BOOST_AUTO_TEST_CASE(testLower)
{
    subject.setLower(true);
    BOOST_CHECK(subject.getLower());

    subject.setLower(false);
    BOOST_CHECK(!subject.getLower());
}

BOOST_AUTO_TEST_CASE(testConstructor)
{
    BOOST_CHECK(subject.getId() == "");
    BOOST_CHECK(subject.getAnalysis() == "");
    BOOST_CHECK(subject.getLevel() == 0);
    BOOST_CHECK(subject.getApart() == false);
    BOOST_CHECK(subject.getMinLevel() == 2);
    BOOST_CHECK(subject.getMaxLevel() == 2);
    BOOST_CHECK(subject.getMaxNo() == 0);
    BOOST_CHECK(subject.getLanguage() == "");
    BOOST_CHECK(subject.getMode() == "");
    BOOST_CHECK(subject.getOption() == "");
    BOOST_CHECK(subject.getSpecialChar() == "");
    BOOST_CHECK(subject.getDictionaryPath() == "");
    BOOST_CHECK(subject.getPrefix() == false);
    BOOST_CHECK(subject.getSuffix() == false);
    BOOST_CHECK(subject.getCaseSensitive() == false);
    BOOST_CHECK(subject.getIdxFlag() == (unsigned char)0x77);
    BOOST_CHECK(subject.getSchFlag() == (unsigned char)0x77);
    BOOST_CHECK(subject.getStem() == true);
    BOOST_CHECK(subject.getLower() == true);
}

BOOST_AUTO_TEST_SUITE_END() // LAConfigUnit_test
