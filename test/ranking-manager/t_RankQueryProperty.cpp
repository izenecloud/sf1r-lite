#include <boost/test/unit_test.hpp>

#include <ranking-manager/RankQueryProperty.h>

using namespace sf1r;

BOOST_AUTO_TEST_SUITE(RankQueryProperty_Terms_test)

BOOST_AUTO_TEST_CASE(Ctor_test)
{
    RankQueryProperty attr;
    BOOST_CHECK(attr.size() == 0);
    BOOST_CHECK(attr.empty());
}

BOOST_AUTO_TEST_CASE(AddTerm_test)
{
    RankQueryProperty attr;
    attr.addTerm(10);

    BOOST_CHECK(attr.size() == 1);
    BOOST_CHECK(!attr.empty());

    BOOST_CHECK(attr.termAt(0) == 10);
    BOOST_CHECK(attr.totalTermFreqAt(0) == 0);
    BOOST_CHECK(attr.documentFreqAt(0) == 0);
    BOOST_CHECK(attr.termFreqAt(0) == 0);
    BOOST_CHECK(attr.termPositionsBeginAt(0) == attr.termPositionsEndAt(0));
}

BOOST_AUTO_TEST_CASE(SetTotalTermFreq_test)
{
    RankQueryProperty attr;
    attr.addTerm(10);
    attr.setTotalTermFreq(5);

    BOOST_CHECK(attr.size() == 1);
    BOOST_CHECK(attr.termAt(0) == 10);
    BOOST_CHECK(attr.totalTermFreqAt(0) == 5);
}

BOOST_AUTO_TEST_CASE(SetDocumentFreq_test)
{
    RankQueryProperty attr;
    attr.addTerm(10);
    attr.setDocumentFreq(5);

    BOOST_CHECK(attr.size() == 1);
    BOOST_CHECK(attr.termAt(0) == 10);
    BOOST_CHECK(attr.documentFreqAt(0) == 5);
}

BOOST_AUTO_TEST_CASE(SetTermFreq_test)
{
    RankQueryProperty attr;
    attr.addTerm(10);
    attr.setTermFreq(5);

    BOOST_CHECK(attr.size() == 1);
    BOOST_CHECK(attr.termAt(0) == 10);
    BOOST_CHECK(attr.termFreqAt(0) == 5);
    BOOST_CHECK(attr.termPositionsBeginAt(0) == attr.termPositionsEndAt(0));
}

BOOST_AUTO_TEST_CASE(PushPosition_test)
{
    RankQueryProperty attr;
    attr.addTerm(10);
    attr.pushPosition(5);

    BOOST_CHECK(attr.size() == 1);
    BOOST_CHECK(attr.termAt(0) == 10);
    BOOST_CHECK(attr.termFreqAt(0) == 1);
    typedef RankQueryProperty::const_position_iterator iterator;
    iterator iter = attr.termPositionsBeginAt(0);
    iterator iterEnd = attr.termPositionsEndAt(0);
    BOOST_CHECK(iter != iterEnd);
    BOOST_CHECK(*iter == 5);
    BOOST_CHECK(++iter == iterEnd);
}

BOOST_AUTO_TEST_CASE(PushMultiplePositions_test)
{
    RankQueryProperty attr;
    attr.addTerm(10);
    attr.pushPosition(5);
    attr.pushPosition(10);

    BOOST_CHECK(attr.size() == 1);
    BOOST_CHECK(attr.termAt(0) == 10);
    BOOST_CHECK(attr.termFreqAt(0) == 2);
    typedef RankQueryProperty::const_position_iterator iterator;
    iterator iter = attr.termPositionsBeginAt(0);
    iterator iterEnd = attr.termPositionsEndAt(0);
    BOOST_CHECK(iter != iterEnd);
    BOOST_CHECK(*iter == 5);
    ++iter;
    BOOST_CHECK(iter != iterEnd);
    BOOST_CHECK(*iter == 10);
    BOOST_CHECK(++iter == iterEnd);
}

BOOST_AUTO_TEST_CASE(SetFreqOverwritePush_test)
{
    RankQueryProperty attr;
    attr.addTerm(10);
    attr.pushPosition(5);

    attr.setTermFreq(5);

    BOOST_CHECK(attr.size() == 1);
    BOOST_CHECK(attr.termAt(0) == 10);
    BOOST_CHECK(attr.termFreqAt(0) == 5);
    BOOST_CHECK(attr.termPositionsBeginAt(0) == attr.termPositionsEndAt(0));
}

BOOST_AUTO_TEST_CASE(PushOverwriteSetFreq_test)
{
    RankQueryProperty attr;
    attr.addTerm(10);
    attr.setTermFreq(5);
    attr.pushPosition(5);

    BOOST_CHECK(attr.size() == 1);
    BOOST_CHECK(attr.termAt(0) == 10);
    BOOST_CHECK(attr.termFreqAt(0) == 1);
    typedef RankQueryProperty::const_position_iterator iterator;
    iterator iter = attr.termPositionsBeginAt(0);
    iterator iterEnd = attr.termPositionsEndAt(0);
    BOOST_CHECK(iter != iterEnd);
    BOOST_CHECK(*iter == 5);
    BOOST_CHECK(++iter == iterEnd);
}

BOOST_AUTO_TEST_SUITE_END() // RankQueryProperty_Terms_test

BOOST_AUTO_TEST_SUITE(RankQueryProperty_Global_test)

BOOST_AUTO_TEST_CASE(Ctor_test)
{
    RankQueryProperty attr;
    BOOST_CHECK(attr.getNumDocs() == 0);
    BOOST_CHECK(attr.getTotalPropertyLength() == 0);
    BOOST_CHECK(attr.getAveragePropertyLength() == 0.0F);
    BOOST_CHECK(attr.getQueryLength() == 0);
}


BOOST_AUTO_TEST_CASE(NumDocs_test)
{
    RankQueryProperty attr;
    attr.setNumDocs(10);
    BOOST_CHECK(attr.getNumDocs() == 10);
}

BOOST_AUTO_TEST_CASE(TotalPropertyLength_test)
{
    RankQueryProperty attr;
    attr.setTotalPropertyLength(10);
    BOOST_CHECK(attr.getTotalPropertyLength() == 10);
}

BOOST_AUTO_TEST_CASE(AveragePropertyLength_test)
{
    RankQueryProperty attr;
    attr.setNumDocs(10);
    attr.setTotalPropertyLength(100);
    BOOST_CHECK(attr.getAveragePropertyLength() - 10.0F < 1E-8F);
}

BOOST_AUTO_TEST_CASE(QueryLength_test)
{
    RankQueryProperty attr;
    attr.setQueryLength(10);
    BOOST_CHECK(attr.getQueryLength() == 10);
}

BOOST_AUTO_TEST_SUITE_END() // RankQueryProperty_Global_test

