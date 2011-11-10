///
/// @file ItemManagerTestFixture.h
/// @brief fixture class to test ItemManager
/// @author Jun Jiang
/// @date Created 2011-11-08
///

#ifndef ORDER_MANAGER_TEST_FIXTURE
#define ORDER_MANAGER_TEST_FIXTURE

#include <string>
#include <boost/scoped_ptr.hpp>

namespace sf1r
{
class OrderManager;

class OrderManagerTestFixture
{
public:
    void setTestDir(const std::string& dir);

    void resetInstance();

    void addOrder(const char* inputStr);

    void checkFreqItems(const char* inputItemStr, const char* goldItemStr);

    void checkFreqBundles();

private:
    std::string testDir_;
    boost::scoped_ptr<OrderManager> orderManager_;
};

} // namespace sf1r

#endif // ORDER_MANAGER_TEST_FIXTURE
