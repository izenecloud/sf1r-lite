#include "ClickCounterDBTestFixture.h"

#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include <set>
#include <algorithm> // min
#include <climits> // INT_MAX

namespace sf1r
{

void ClickCounterDBTestFixture::setTestData(int keyNum, int valueNum)
{
    keyDistrib_.param(DistribType::param_type(1, keyNum));
    valueDistrib_.param(DistribType::param_type(1, valueNum));
    limitDistrib_.param(DistribType::param_type(1, valueNum * 1.2));
}

void ClickCounterDBTestFixture::resetInstance(const std::string& path)
{
    BOOST_TEST_MESSAGE("reset db instance");

    // flush first
    db_.reset();
    db_.reset(new DBType(path));
}

void ClickCounterDBTestFixture::runClick(int times)
{
    BOOST_TEST_MESSAGE("running clicks " << times << " times");

    for (int i=0; i<times; ++i)
    {
        int key = keyDistrib_(keyEngine_);
        std::string keyStr = "key_" + boost::lexical_cast<std::string>(key);
        int value = valueDistrib_(valueEngine_);

        ++keyValueCounter_[keyStr][value];

        ClickCounter clickCounter;
        BOOST_CHECK(db_->get(keyStr, clickCounter));

        clickCounter.click(value);

        BOOST_CHECK(db_->update(keyStr, clickCounter));
    }
}

void ClickCounterDBTestFixture::checkCount()
{
    BOOST_TEST_MESSAGE("checking " << keyValueCounter_.size() << " keys");

    for (KeyValueCounter::const_iterator keyIt = keyValueCounter_.begin();
        keyIt != keyValueCounter_.end(); ++keyIt)
    {
        const string& keyStr = keyIt->first;

        checkClickCounter_(keyStr);
    }
}

void ClickCounterDBTestFixture::checkClickCounter_(const std::string& keyStr)
{
    int limit = limitDistrib_(limitEngine_);

    std::vector<ClickCounter::value_type> values;
    std::vector<ClickCounter::freq_type> freqs;
    ClickCounter clickCounter;

    BOOST_CHECK(db_->get(keyStr, clickCounter));
    clickCounter.getFreqClick(limit, values, freqs);

    const ValueCounter& valueCounter = keyValueCounter_[keyStr];
    const int resultNum = values.size();
    const int totalValueNum = valueCounter.size();

    BOOST_CHECK_EQUAL(resultNum, freqs.size());
    BOOST_CHECK_EQUAL(resultNum, std::min(totalValueNum, limit));

    std::set<int> checkedSet;
    int maxFreq = INT_MAX;
    for (int i=0; i<resultNum; ++i)
    {
        const int value = values[i];
        const int freq = freqs[i];

        ValueCounter::const_iterator findIt = valueCounter.find(value);
        BOOST_CHECK(findIt != valueCounter.end());
        BOOST_CHECK_EQUAL(freq, findIt->second);

        // not checked before
        BOOST_CHECK(checkedSet.insert(value).second);

        // decreasing freq
        BOOST_CHECK_LE(freq, maxFreq);
        maxFreq = freq;
    }

    // check rest <= maxFreq
    int totalFreq = 0;
    for (ValueCounter::const_iterator mapIt = valueCounter.begin();
        mapIt != valueCounter.end(); ++mapIt)
    {
        totalFreq += mapIt->second;

        if (checkedSet.find(mapIt->first) == checkedSet.end())
        {
            BOOST_CHECK_LE(mapIt->second, maxFreq);
        }
    }

    BOOST_CHECK_EQUAL(clickCounter.getTotalFreq(), totalFreq);

    /*BOOST_TEST_MESSAGE("check key: " << keyStr << ", limit: " << limit <<
                       ", resultNum: " << resultNum << ", totalValueNum: " << totalValueNum <<
                       ", totalFreq: " << totalFreq);*/
}

} // namespace sf1r
