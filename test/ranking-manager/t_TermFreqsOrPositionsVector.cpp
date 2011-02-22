#include <boost/test/unit_test.hpp>

#include <ranking-manager/TermFreqsOrPositionsVector.h>

using namespace sf1r;

BOOST_AUTO_TEST_SUITE(TermFreqsOrPositionsVector_test)

BOOST_AUTO_TEST_CASE(DefaultCtor_test)
{
    TermFreqsOrPositionsVector v;
    BOOST_CHECK(v.size() == 0);
    BOOST_CHECK(v.empty());
}


BOOST_AUTO_TEST_CASE(Ctor_test)
{
    TermFreqsOrPositionsVector v(5);
    BOOST_CHECK(v.size() == 5);
    BOOST_CHECK(!v.empty());
}

BOOST_AUTO_TEST_CASE(EmptyVectorHasNoActivatedTerms_test)
{
    TermFreqsOrPositionsVector v(3);
    BOOST_CHECK(!v.activated(0));
    BOOST_CHECK(!v.activated(1));
    BOOST_CHECK(!v.activated(2));

    BOOST_CHECK(v.getActive() == TermFreqsOrPositionsVector::kNotPosition);
}

BOOST_AUTO_TEST_CASE(ActivateFirst_test)
{
    TermFreqsOrPositionsVector v(3);
    v.activate(0);
    BOOST_CHECK(v.activated(0));
    BOOST_CHECK(!v.activated(1));
    BOOST_CHECK(!v.activated(2));

    BOOST_CHECK(v.getActive() == 0);
}

BOOST_AUTO_TEST_CASE(ActivateMultiple_test)
{
    TermFreqsOrPositionsVector v(3);
    v.activate(0);
    v.activate(2);
    BOOST_CHECK(v.activated(0));
    BOOST_CHECK(!v.activated(1));
    BOOST_CHECK(v.activated(2));

    BOOST_CHECK(v.getActive() == 2);
}

BOOST_AUTO_TEST_CASE(NewActivatedTermIsEmpty_test)
{
    TermFreqsOrPositionsVector v(3);
    v.activate(0);
    BOOST_CHECK(v.freqAt(0) == 0);
}

BOOST_AUTO_TEST_CASE(Push_test)
{
    TermFreqsOrPositionsVector v(3);
    v.activate(0);

    v.push(1);
    BOOST_CHECK(v.freqAt(0) == 1);

    TermFreqsOrPositionsVector::position_iterator iter = v.beginAt(0);
    TermFreqsOrPositionsVector::position_iterator iterEnd = v.endAt(0);
    BOOST_CHECK(iter != iterEnd);
    BOOST_CHECK(*iter == 1);
    BOOST_CHECK(++iter == iterEnd);
}

BOOST_AUTO_TEST_CASE(PushForTwoTerms_test)
{
    TermFreqsOrPositionsVector v(3);
    v.activate(1);
    v.push(2);

    v.activate(0);
    v.push(1);

    {
        BOOST_CHECK(v.freqAt(0) == 1);
        TermFreqsOrPositionsVector::position_iterator iter = v.beginAt(0);
        TermFreqsOrPositionsVector::position_iterator iterEnd = v.endAt(0);
        BOOST_CHECK(iter != iterEnd);
        BOOST_CHECK(*iter == 1);
        BOOST_CHECK(++iter == iterEnd);
    }

    {
        BOOST_CHECK(v.freqAt(1) == 1);
        TermFreqsOrPositionsVector::position_iterator iter = v.beginAt(1);
        TermFreqsOrPositionsVector::position_iterator iterEnd = v.endAt(1);
        BOOST_CHECK(iter != iterEnd);
        BOOST_CHECK(*iter == 2);
        BOOST_CHECK(++iter == iterEnd);
    }
}

BOOST_AUTO_TEST_CASE(Overwrite_test)
{
    TermFreqsOrPositionsVector v(3);
    v.activate(0);
    v.push(1);

    v.activate(1);
    v.activate(0);
    BOOST_CHECK(v.freqAt(0) == 0);
    BOOST_CHECK(v.beginAt(0) == v.endAt(0));

    v.push(2);

    {
        BOOST_CHECK(v.freqAt(0) == 1);
        TermFreqsOrPositionsVector::position_iterator iter = v.beginAt(0);
        TermFreqsOrPositionsVector::position_iterator iterEnd = v.endAt(0);
        BOOST_CHECK(iter != iterEnd);
        BOOST_CHECK(*iter == 2);
        BOOST_CHECK(++iter == iterEnd);
    }
}

BOOST_AUTO_TEST_CASE(Reset_test)
{
    TermFreqsOrPositionsVector v(3);
    v.activate(0);
    v.push(1);

    v.reset();
    BOOST_CHECK(v.size() == 0);

    BOOST_CHECK(v.getActive() == TermFreqsOrPositionsVector::kNotPosition);
}

BOOST_AUTO_TEST_CASE(SetFreq_test)
{
    TermFreqsOrPositionsVector v(3);
    v.setFreq(0, 3);

    BOOST_CHECK(!v.activated(0));
    BOOST_CHECK(v.freqAt(0) == 3);
    BOOST_CHECK(v.beginAt(0) == v.endAt(0));
}

BOOST_AUTO_TEST_CASE(SetFreqOverwrite_test)
{
    TermFreqsOrPositionsVector v(3);
    v.activate(0);
    v.push(1);

    v.setFreq(0, 3);
    BOOST_CHECK(!v.activated(0));
    BOOST_CHECK(v.freqAt(0) == 3);
    BOOST_CHECK(v.beginAt(0) == v.endAt(0));
    BOOST_CHECK(v.getActive() == TermFreqsOrPositionsVector::kNotPosition);
}

BOOST_AUTO_TEST_CASE(SetFreqOverwriteNotCurrentActive_test)
{
    TermFreqsOrPositionsVector v(3);
    v.activate(0);
    v.push(1);
    v.activate(1);
    v.push(2);

    v.setFreq(0, 3);
    BOOST_CHECK(!v.activated(0));
    BOOST_CHECK(v.freqAt(0) == 3);
    BOOST_CHECK(v.beginAt(0) == v.endAt(0));

    v.push(3);
    BOOST_CHECK(v.getActive() == 1);
    BOOST_CHECK(v.activated(1));
    BOOST_CHECK(v.freqAt(1) == 2);
    TermFreqsOrPositionsVector::position_iterator iter = v.beginAt(1);
    TermFreqsOrPositionsVector::position_iterator iterEnd = v.endAt(1);
    BOOST_CHECK(iter != iterEnd);
    BOOST_CHECK(*iter == 2);
    ++iter;
    BOOST_CHECK(*iter == 3);
    ++iter;
    BOOST_CHECK(iter == iterEnd);
}

BOOST_AUTO_TEST_CASE(AddTerm_test)
{
    TermFreqsOrPositionsVector v(3);
    v.addTerm();
    v.push(1);

    BOOST_CHECK(v.getActive() == 3);
    BOOST_CHECK(v.freqAt(3) == 1);
    TermFreqsOrPositionsVector::position_iterator iter = v.beginAt(3);
    TermFreqsOrPositionsVector::position_iterator iterEnd = v.endAt(3);
    BOOST_CHECK(iter != iterEnd);
    BOOST_CHECK(*iter == 1);
    ++iter;
    BOOST_CHECK(iter == iterEnd);
}

BOOST_AUTO_TEST_CASE(ActiveCurrentActivated_test)
{
    TermFreqsOrPositionsVector v(3);
    v.activate(0);

    v.push(1);

    v.activate(0);
    v.push(2);

    BOOST_CHECK(v.freqAt(0) == 2);

    TermFreqsOrPositionsVector::position_iterator iter = v.beginAt(0);
    TermFreqsOrPositionsVector::position_iterator iterEnd = v.endAt(0);
    BOOST_CHECK(iter != iterEnd);
    BOOST_CHECK(*iter == 1);
    ++iter;
    BOOST_CHECK(*iter == 2);
    BOOST_CHECK(++iter == iterEnd);
}

BOOST_AUTO_TEST_CASE(InactivatedPositionsIterator_test)
{
    TermFreqsOrPositionsVector v(3);
    BOOST_CHECK(v.beginAt(0) == v.endAt(0));
}

BOOST_AUTO_TEST_CASE(OnlyFreqStoredTermPositionsIterator_test)
{
    TermFreqsOrPositionsVector v(3);
    v.setFreq(0, 3);
    BOOST_CHECK(v.beginAt(0) == v.endAt(0));
}

BOOST_AUTO_TEST_CASE(ResizeShrink_test)
{
    TermFreqsOrPositionsVector v(3);
    v.activate(0);
    v.push(1);
    v.activate(1);
    v.push(2);

    v.resize(1);
    BOOST_CHECK(v.size() == 1);
    BOOST_CHECK(v.freqAt(0) == 1);

    TermFreqsOrPositionsVector::position_iterator iter = v.beginAt(0);
    TermFreqsOrPositionsVector::position_iterator iterEnd = v.endAt(0);
    BOOST_CHECK(iter != iterEnd);
    BOOST_CHECK(*iter == 1);
    BOOST_CHECK(++iter == iterEnd);
}

BOOST_AUTO_TEST_CASE(ResizeDeactivate_test)
{
    TermFreqsOrPositionsVector v(3);
    v.activate(2);

    v.resize(1);
    BOOST_CHECK(v.getActive() == TermFreqsOrPositionsVector::kNotPosition);
}

BOOST_AUTO_TEST_CASE(ResizeExpand_test)
{
    TermFreqsOrPositionsVector v;
    v.resize(1);

    BOOST_CHECK(!v.activated(0));
    BOOST_CHECK(0 == v.freqAt(0));
}

BOOST_AUTO_TEST_SUITE_END() // TermFreqsOrPositionsVector_test
