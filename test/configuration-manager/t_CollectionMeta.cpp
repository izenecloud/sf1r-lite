/**
 * @file t_CollectionMeta.cpp
 * @author Ian Yang
 * @date Created <2010-08-23 14:40:56>
 */
#include <boost/test/unit_test.hpp>
#include <configuration-manager/CollectionMeta.h>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include <sstream>

using namespace sf1r;

namespace { // {anonymous}
class CollectionMetaFixture
{
public:
    typedef CollectionMeta::property_config_const_iterator const_iterator;

    CollectionMeta meta;
};
} // namespace {anonymous}

BOOST_FIXTURE_TEST_SUITE(CollectionMeta_test, CollectionMetaFixture)

BOOST_AUTO_TEST_CASE(ColId_test)
{
    meta.setColId(1000);
    BOOST_CHECK(meta.getColId() == 1000);

    meta.setColId(0);
    BOOST_CHECK(meta.getColId() == 0);
}

BOOST_AUTO_TEST_CASE(Name_test)
{
    meta.setName("test");
    BOOST_CHECK(meta.getName() == "test");
}

BOOST_AUTO_TEST_CASE(Encoding_test)
{
    meta.setEncoding(izenelib::util::UString::GB2312);
    BOOST_CHECK(meta.getEncoding() == izenelib::util::UString::GB2312);
}

BOOST_AUTO_TEST_CASE(WildcardType_test)
{
    meta.setWildcardType("unknown");
    BOOST_CHECK(meta.getWildcardType() == "unknown");
    BOOST_CHECK(!meta.isUnigramWildcard());
    BOOST_CHECK(!meta.isTrieWildcard());

    meta.setWildcardType("unigram");
    BOOST_CHECK(meta.getWildcardType() == "unigram");
    BOOST_CHECK(meta.isUnigramWildcard());
    BOOST_CHECK(!meta.isTrieWildcard());

    meta.setWildcardType("trie");
    BOOST_CHECK(meta.getWildcardType() == "trie");
    BOOST_CHECK(!meta.isUnigramWildcard());
    BOOST_CHECK(meta.isTrieWildcard());
}

BOOST_AUTO_TEST_CASE(GeneralClassfierLanguage_test)
{
    meta.setGeneralClassifierLanguage("Chinese");
    BOOST_CHECK(meta.getGeneralClassifierLanguage() == "Chinese");
}

BOOST_AUTO_TEST_CASE(RankingMethod_test)
{
    meta.setRankingMethod("km");
    BOOST_CHECK(meta.getRankingMethod() == "km");
}

BOOST_AUTO_TEST_CASE(DateFormat_test)
{
    meta.setDateFormat("%Y%m%d");
    BOOST_CHECK(meta.getDateFormat() == "%Y%m%d");
}

BOOST_AUTO_TEST_CASE(InsertPropertyConfig_test)
{
    PropertyConfig title;
    title.setName("title");

    PropertyConfig content;
    content.setName("content");

    BOOST_CHECK(meta.insertPropertyConfig(title));
    BOOST_CHECK(meta.insertPropertyConfig(content));

    PropertyConfig foundResult;

    BOOST_CHECK(meta.getPropertyConfig("title", foundResult));
    BOOST_CHECK(foundResult.getName() == "title");

    BOOST_CHECK(meta.getPropertyConfig("content", foundResult));
    BOOST_CHECK(foundResult.getName() == "content");

    BOOST_CHECK(meta.getDocumentSchema().size() == 2);
}

BOOST_AUTO_TEST_CASE(InsertExistPropertyConfig_test)
{
    PropertyConfig title;
    title.setIsSummary(true);
    title.setName("title");

    PropertyConfig title2;
    title2.setName("title");
    title2.setIsSummary(false);

    meta.insertPropertyConfig(title);

    BOOST_CHECK(!meta.insertPropertyConfig(title2));

    PropertyConfig foundResult;

    BOOST_CHECK(meta.getPropertyConfig("title", foundResult));
    BOOST_CHECK(foundResult.getIsSummary());
    BOOST_CHECK(foundResult.getName() == "title");

    BOOST_CHECK(meta.getDocumentSchema().size() == 1);
}

BOOST_AUTO_TEST_CASE(PropertyConfigIterator_test)
{
    const_iterator it = meta.propertyConfigBegin();
    BOOST_CHECK(it == meta.propertyConfigEnd());

    PropertyConfig title;
    title.setName("title");

    PropertyConfig content;
    content.setName("content");

    BOOST_CHECK(meta.insertPropertyConfig(title));
    BOOST_CHECK(meta.insertPropertyConfig(content));

    it = meta.propertyConfigBegin();
    BOOST_CHECK(it != meta.propertyConfigEnd());
    BOOST_CHECK(it->getName() == "content");

    ++it;
    BOOST_CHECK(it != meta.propertyConfigEnd());
    BOOST_CHECK(it->getName() == "title");

    ++it;
    BOOST_CHECK(it == meta.propertyConfigEnd());
}

BOOST_AUTO_TEST_CASE(NumberPropertyConfig_test)
{
    // should not crash
    meta.numberPropertyConfig();

    PropertyConfig title;
    title.setName("title");
    meta.insertPropertyConfig(title);
    meta.numberPropertyConfig();

    BOOST_CHECK(meta.propertyConfigBegin()->getPropertyId() == 1);

    PropertyConfig content;
    content.setName("content");
    meta.insertPropertyConfig(content);
    meta.numberPropertyConfig();

    const_iterator it = meta.propertyConfigBegin();
    BOOST_CHECK(it->getPropertyId() == 1);
    ++it;
    BOOST_CHECK(it->getPropertyId() == 2);
}

BOOST_AUTO_TEST_CASE(ClearPropertyConfig_test)
{
    PropertyConfig title;
    title.setName("title");
    PropertyConfig content;
    content.setName("content");
    meta.insertPropertyConfig(title);
    meta.insertPropertyConfig(content);

    BOOST_CHECK(meta.getDocumentSchema().size() == 2);

    meta.clearPropertyConfig();
    BOOST_CHECK(meta.getDocumentSchema().size() == 0);
}

// BOOST_AUTO_TEST_CASE(Serialization_test)
// {
//     meta.setColId(10);
//     meta.setName("name");
//     meta.setEncoding(izenelib::util::UString::UTF_8);
//     meta.setNeedACL(true);
//     meta.setWildcardType("unigram");
//     meta.setGeneralClassifierLanguage("CN");
//     meta.setRankingMethod("km");
//     meta.setDateFormat("%Y");
// 
//     CollectionPath path;
//     path.setBasePath("base");
//     path.setScdPath("scd");
//     meta.setCollectionPath(path);
// 
//     PropertyConfig title;
//     title.setName("title");
//     PropertyConfig content;
//     content.setName("content");
//     meta.insertPropertyConfig(title);
//     meta.insertPropertyConfig(content);
// 
//     std::stringstream ss(std::ios_base::binary |
//                          std::ios_base::out |
//                          std::ios_base::in);
//     {
//         boost::archive::binary_oarchive oa(ss, boost::archive::no_header);
//         oa << meta;
//     }
// 
//     {
//         CollectionMeta clonedMeta;
//         boost::archive::binary_iarchive ia(ss, boost::archive::no_header);
//         ia >> clonedMeta;
// 
//         BOOST_CHECK(clonedMeta.getColId() == 10);
//         BOOST_CHECK(clonedMeta.getName() == "name");
//         BOOST_CHECK(clonedMeta.getEncoding() == izenelib::util::UString::UTF_8);
//         BOOST_CHECK(clonedMeta.needACL());
//         BOOST_CHECK(clonedMeta.getWildcardType() == "unigram");
//         BOOST_CHECK(clonedMeta.getGeneralClassifierLanguage() == "CN");
//         BOOST_CHECK(clonedMeta.getRankingMethod() == "km");
//         BOOST_CHECK(clonedMeta.getDateFormat() == "%Y");
//         BOOST_CHECK(clonedMeta.getCollectionPath().getBasePath() == "base/");
//         BOOST_CHECK(clonedMeta.getDocumentSchema().size() == 2);
// 
//         const_iterator it = clonedMeta.propertyConfigBegin();
//         BOOST_CHECK(it != clonedMeta.propertyConfigEnd());
//         BOOST_CHECK(it->getName() == "content");
//         ++it;
//         BOOST_CHECK(it != clonedMeta.propertyConfigEnd());
//         BOOST_CHECK(it->getName() == "title");
//         ++it;
//         BOOST_CHECK(it == clonedMeta.propertyConfigEnd());
//     }
// }


BOOST_AUTO_TEST_SUITE_END() // CollectionMeta_test
