/// @file FContainerTestFixture.h
/// @brief fixture class used in t_fcontainer_febird.cpp
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-06-01
///

#ifndef SF1R_FCONTAINER_TEST_FIXTURE_H
#define SF1R_FCONTAINER_TEST_FIXTURE_H

#include <mining-manager/util/fcontainer_febird.h>
#include <boost/test/unit_test.hpp>
#include <string>

namespace sf1r
{

class FContainerTestFixture
{
public:
    FContainerTestFixture();

    template <class ContainerT>
    void checkContainerSerialize(
        const std::string& fileName,
        const ContainerT& container
    );

    template <class ContainerT>
    void checkEmptyFile(const std::string& fileName);

protected:
    std::string testDir_;
};

template <class ContainerT>
void FContainerTestFixture::checkContainerSerialize(
    const std::string& fileName,
    const ContainerT& container
)
{
    unsigned int saveCount = 0;
    BOOST_CHECK(save_container_febird(testDir_, fileName,
                                      container, saveCount));
    BOOST_CHECK_EQUAL(saveCount, container.size());

    ContainerT loadContainer;
    unsigned int loadCount = 0;
    BOOST_CHECK(load_container_febird(testDir_, fileName,
                                      loadContainer, loadCount));
    BOOST_CHECK_EQUAL(loadCount, container.size());
    BOOST_CHECK_EQUAL_COLLECTIONS(loadContainer.begin(), loadContainer.end(),
                                  container.begin(), container.end());
}

template <class ContainerT>
void FContainerTestFixture::checkEmptyFile(const std::string& fileName)
{
    BOOST_TEST_MESSAGE("load unexisting file " << fileName);
    ContainerT container;
    unsigned int loadCount = 0;
    BOOST_CHECK(load_container_febird(testDir_, fileName,
                                      container, loadCount));
    BOOST_CHECK_EQUAL(loadCount, 0U);
    BOOST_CHECK_EQUAL(container.size(), 0U);

    BOOST_TEST_MESSAGE("save empty container to file " << fileName);
    unsigned int saveCount = 0;
    BOOST_CHECK(save_container_febird(testDir_, fileName,
                                      container, saveCount));
    BOOST_CHECK_EQUAL(saveCount, 0U);

    BOOST_TEST_MESSAGE("load empty container from file " << fileName);
    ContainerT loadContainer;
    BOOST_CHECK(load_container_febird(testDir_, fileName,
                                      loadContainer, loadCount));
    BOOST_CHECK_EQUAL(loadCount, 0U);
    BOOST_CHECK_EQUAL(loadContainer.size(), 0U);
}

} // namespace sf1r

#endif // SF1R_FCONTAINER_TEST_FIXTURE_H
