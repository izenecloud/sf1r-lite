/**
 * @file t_Acl.cpp
 * @author Ian Yang
 * @date Created <2010-08-23 15:02:35>
 */
#include <boost/test/unit_test.hpp>
#include <configuration-manager/Acl.h>

#include <algorithm>

using sf1r::Acl;

namespace { // {anonymous}
class AclFixture
{
public:
    typedef Acl::token_set_type token_set_type;

    Acl acl;
    token_set_type tokens;
    token_set_type expect;
};
} // namespace {anonymous}

BOOST_FIXTURE_TEST_SUITE(Acl_test, AclFixture)

BOOST_AUTO_TEST_CASE(testInsertEmptyToken)
{
    Acl::insertTokensTo("", tokens);
    BOOST_CHECK(expect == tokens);
}

BOOST_AUTO_TEST_CASE(testInsertSingleToken)
{
    expect.insert("abc");

    Acl::insertTokensTo("abc", tokens);
    BOOST_CHECK(expect == tokens);
}

BOOST_AUTO_TEST_CASE(testInsertSingleExistedToken)
{
    expect.insert("abc");
    tokens.insert("abc");

    Acl::insertTokensTo("abc", tokens);
    BOOST_CHECK(expect == tokens);
}

BOOST_AUTO_TEST_CASE(testInsertMultipleTokens)
{
    expect.insert("abc");
    expect.insert("def");

    Acl::insertTokensTo("def,abc", tokens);
    BOOST_CHECK(expect == tokens);
}

BOOST_AUTO_TEST_CASE(testAllow)
{
    expect.insert("a");
    expect.insert("b");

    acl.allow("a,b");
    BOOST_CHECK(std::equal(acl.allowedTokensBegin(), acl.allowedTokensEnd(), expect.begin()));
    BOOST_CHECK(acl.deniedTokensBegin() == acl.deniedTokensEnd());
}

BOOST_AUTO_TEST_CASE(testDeny)
{
    expect.insert("a");
    expect.insert("b");

    acl.deny("a,b");
    BOOST_CHECK(std::equal(acl.deniedTokensBegin(), acl.deniedTokensEnd(), expect.begin()));
    BOOST_CHECK(acl.allowedTokensBegin() == acl.allowedTokensEnd());
}

BOOST_AUTO_TEST_CASE(testCheckEmpty)
{
    BOOST_CHECK(acl.check(""));
    BOOST_CHECK(acl.check("ceo"));
}

BOOST_AUTO_TEST_CASE(testCheckOnlyAllow)
{
    acl.allow("ceo");

    BOOST_CHECK(!acl.check(""));
    BOOST_CHECK(!acl.check("hr"));
    BOOST_CHECK(acl.check("ceo"));
}

BOOST_AUTO_TEST_CASE(testCheckOnlyDeny)
{
    acl.deny("ceo");

    BOOST_CHECK(acl.check(""));
    BOOST_CHECK(acl.check("hr"));
    BOOST_CHECK(!acl.check("ceo"));
}

BOOST_AUTO_TEST_CASE(testCheckAllowAndDeny)
{
    acl.allow("ceo");
    acl.deny("hr");

    BOOST_CHECK(!acl.check(""));
    BOOST_CHECK(!acl.check("dev"));
    BOOST_CHECK(!acl.check("hr"));
    BOOST_CHECK(acl.check("ceo"));
    BOOST_CHECK(acl.check("ceo,dev"));
    BOOST_CHECK(!acl.check("ceo,hr"));
}

BOOST_AUTO_TEST_CASE(testMultipleAllow)
{
    acl.allow("ceo,hr");
    BOOST_CHECK(acl.check("ceo"));
    BOOST_CHECK(acl.check("hr"));
    BOOST_CHECK(!acl.check("dev"));
    BOOST_CHECK(acl.check("ceo,hr"));
    BOOST_CHECK(acl.check("ceo,dev"));
    BOOST_CHECK(acl.check("dev,hr"));
    BOOST_CHECK(!acl.check("dev,hrr"));
}

BOOST_AUTO_TEST_CASE(testMultipleDeny)
{
    acl.deny("ceo,hr");

    BOOST_CHECK(!acl.check("ceo"));
    BOOST_CHECK(!acl.check("hr"));
    BOOST_CHECK(!acl.check("hr,ceo"));
    BOOST_CHECK(!acl.check("ceo,hr"));
    BOOST_CHECK(!acl.check("ceo,dev"));
    BOOST_CHECK(!acl.check("dev,hr"));
    BOOST_CHECK(acl.check("dev"));
    BOOST_CHECK(acl.check("dev,hrr"));
}

BOOST_AUTO_TEST_CASE(testMultipleAllowAndDeny)
{
    acl.allow("ceo,hr");
    acl.deny("inactive,intern");

    BOOST_CHECK(acl.check("ceo,hr"));
    BOOST_CHECK(!acl.check("hr,inactive"));
    BOOST_CHECK(acl.check("hr,dev"));
    BOOST_CHECK(!acl.check("dev,inactive"));
}

BOOST_AUTO_TEST_SUITE_END() // Acl_test
