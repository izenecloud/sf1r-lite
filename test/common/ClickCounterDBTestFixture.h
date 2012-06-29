/**
 * @file ClickCounterDBTestFixture.h
 * @brief fixture to test ClickCounter and serialize it into db storage.
 * @author Jun Jiang
 * @date 2012-03-16
 */

#ifndef SF1R_CLICK_COUNTER_DB_TEST_FIXTURE_H
#define SF1R_CLICK_COUNTER_DB_TEST_FIXTURE_H

#include <common/SDBWrapper.h>
#include <common/ClickCounter.h>

#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

namespace sf1r
{

class ClickCounterDBTestFixture
{
public:
    void setTestData(int keyNum, int valueNum);

    void resetInstance(const std::string& path);

    void runClick(int times);

    void checkCount();

private:
    void checkClickCounter_(const std::string& keyStr);

protected:
    typedef ClickCounter<int, int> ClickCounterT;
    typedef SDBWrapper<std::string, ClickCounterT> DBType;
    boost::scoped_ptr<DBType> db_;

    typedef std::map<int, int> ValueCounter; /// value -> freq
    typedef std::map<std::string, ValueCounter> KeyValueCounter;
    KeyValueCounter keyValueCounter_;

    typedef boost::random::uniform_int_distribution<> DistribType;
    boost::random::mt19937 keyEngine_;
    DistribType keyDistrib_;

    boost::random::mt11213b valueEngine_;
    DistribType valueDistrib_;

    boost::random::mt19937 limitEngine_;
    DistribType limitDistrib_;
};

} // namespace sf1r

#endif // SF1R_CLICK_COUNTER_DB_TEST_FIXTURE_H
