/**
 * @file t_CollectionPath.cpp
 * @author Ian Yang
 * @date Created <2010-08-23 12:19:40>
 */
#include <boost/test/unit_test.hpp>
#include <configuration-manager/CollectionPath.h>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include <string>
#include <sstream>

#ifdef WIN32
static const std::string gSlash = "\\";
#else
static const std::string gSlash = "/";
#endif

using sf1r::CollectionPath;

BOOST_AUTO_TEST_SUITE(CollectionPath_test)

BOOST_AUTO_TEST_CASE(TrailingSlashIsAppended_test)
{
    CollectionPath path;

    path.setBasePath("test");
    BOOST_CHECK(path.getBasePath() == "test" + gSlash);

    path.setScdPath("test_scd");
    BOOST_CHECK(path.getScdPath() == "test_scd" + gSlash);

    path.setCollectionDataPath("test_collectiondata");
    BOOST_CHECK(path.getCollectionDataPath() == "test_collectiondata" + gSlash);

    path.setQueryDataPath("test_querydata");
    BOOST_CHECK(path.getQueryDataPath() == "test_querydata" + gSlash);

}

BOOST_AUTO_TEST_CASE(TrailingSlashIsNotAppended_test)
{
    CollectionPath path;

    path.setBasePath("test" + gSlash);
    BOOST_CHECK(path.getBasePath() == "test" + gSlash);

    path.setScdPath("test_scd" + gSlash);
    BOOST_CHECK(path.getScdPath() == "test_scd" + gSlash);

    path.setCollectionDataPath("test_collectiondata" + gSlash);
    BOOST_CHECK(path.getCollectionDataPath() == "test_collectiondata" + gSlash);

    path.setQueryDataPath("test_querydata" + gSlash);
    BOOST_CHECK(path.getQueryDataPath() == "test_querydata" + gSlash);
}

BOOST_AUTO_TEST_CASE(EmptyBaseIsCurrentDir_test)
{
    CollectionPath path;
    path.setBasePath("");

    BOOST_CHECK(path.getBasePath() == "." + gSlash);
}

BOOST_AUTO_TEST_CASE(EmptyPathIsDefaultDirRelativeToBase_test)
{
    CollectionPath path;
    path.setBasePath("base");

    path.setScdPath("");
    BOOST_CHECK(path.getScdPath() == "base" + gSlash + "scd" + gSlash);

    path.setCollectionDataPath("");
    BOOST_CHECK(path.getCollectionDataPath() == "base" + gSlash + "collection-data" + gSlash);

    path.setQueryDataPath("");
    BOOST_CHECK(path.getQueryDataPath() == "base" + gSlash + "query-data" + gSlash);

}

BOOST_AUTO_TEST_CASE(AbsolutePath_test)
{
    CollectionPath path;
    path.setBasePath("base");

    path.setScdPath("scd");
    BOOST_CHECK(path.getScdPath() == "scd" + gSlash);

    path.setCollectionDataPath("collection-data");
    BOOST_CHECK(path.getCollectionDataPath() == "collection-data" + gSlash);

    path.setQueryDataPath("query-data");
    BOOST_CHECK(path.getQueryDataPath() == "query-data" + gSlash);

}

BOOST_AUTO_TEST_CASE(Serialization_test)
{
    CollectionPath path;
    path.setBasePath("base");
    path.setScdPath("scd");
    path.setCollectionDataPath("collection-data");
    path.setQueryDataPath("query-data");
    
    std::stringstream ss(std::ios_base::binary |
                         std::ios_base::out |
                         std::ios_base::in);
    {
        boost::archive::binary_oarchive oa(ss, boost::archive::no_header);
        oa << path;
    }

    {
        CollectionPath clonedPath;
        boost::archive::binary_iarchive ia(ss, boost::archive::no_header);
        ia >> clonedPath;

        BOOST_CHECK(clonedPath.getScdPath() == "scd" + gSlash);
        BOOST_CHECK(clonedPath.getCollectionDataPath() == "collection-data" + gSlash);
        BOOST_CHECK(clonedPath.getQueryDataPath() == "query-data" + gSlash);
    }
}

BOOST_AUTO_TEST_SUITE_END() // CollectionPath_test
