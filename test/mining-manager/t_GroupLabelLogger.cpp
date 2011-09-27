///
/// @file t_GroupLabelLogger.cpp
/// @brief test getting frequent group labels
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-07-26
///

#include <mining-manager/group-label-logger/GroupLabelLogger.h>

#include <string>
#include <map>
#include <set>
#include <algorithm> // min

#include <climits> // INT_MAX

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/random.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace boost;
using namespace sf1r;

namespace bfs = boost::filesystem;

namespace
{
const char* TEST_DIR_STR = "group_label_log_test";
const char* PROP_NAME_STR = "CATEGORY";

class LogChecker
{
private:
    // param "query" of GroupLabelLogger::logLabel()
    boost::mt19937 engine_;
    boost::uniform_int<> queryDistribution_;
    boost::variate_generator<mt19937, uniform_int<> > queryRandom_;

    // param "propValue" of GroupLabelLogger::logLabel()
    boost::mt11213b groupEngine_;
    boost::uniform_int<> groupDistribution_;
    boost::variate_generator<mt11213b, uniform_int<> > groupRandom_;

    // param "limit" of GroupLabelLogger::getFreqLabel()
    boost::uniform_int<> limitDistribution_;
    boost::variate_generator<mt19937, uniform_int<> > limitRandom_;

    typedef map<int, int> GroupCountMap;
    struct GroupCounter
    {
        GroupCountMap countMap_;
        int manualTop_;

        GroupCounter()
            : manualTop_(0)
        {}
    };

    typedef map<string, GroupCounter> QueryGroupMap;
    QueryGroupMap queryGroupMap_;

private:
    void checkGroup_(
        int limit,
        const std::vector<LabelCounter::value_type>& valueVec,
        const std::vector<int>& freqVec,
        const GroupCounter& groupCounter
    )
    {
        //cout << "checkGroup_(), limit: " << limit
             //<< ", valueVec.size(): " << valueVec.size();
        //for (size_t i=0; i<valueVec.size(); ++i)
        //{
            //cout << ", (" << valueVec[i] << ", " << freqVec[i] << ")";
        //}
        //cout << endl;


        BOOST_CHECK_EQUAL(valueVec.size(), freqVec.size());
        BOOST_CHECK_LE(static_cast<int>(valueVec.size()), limit);

        set<int> checkedSet;
        size_t i = 0;
        const GroupCountMap& groupCountMap = groupCounter.countMap_;
        int totalCount = groupCountMap.size();

        // check top label set manually
        if (groupCounter.manualTop_)
        {
            int group = groupCounter.manualTop_;
            BOOST_CHECK_EQUAL(valueVec[0], group);

            GroupCountMap::const_iterator findIt = groupCountMap.find(group);
            if (findIt != groupCountMap.end())
            {
                BOOST_CHECK_EQUAL(freqVec[0], findIt->second);
            }
            else
            {
                BOOST_CHECK_EQUAL(freqVec[0], 0);
                ++totalCount;
            }

            BOOST_CHECK(checkedSet.insert(group).second);

            ++i;
        }

        // check result size
        BOOST_CHECK_EQUAL(static_cast<int>(valueVec.size()), min(totalCount, limit));

        int maxFreq = INT_MAX;
        for (; i<valueVec.size(); ++i)
        {
            int group = valueVec[i];
            const int freqValue = freqVec[i];

            // freq value
            GroupCountMap::const_iterator findIt = groupCountMap.find(group);
            BOOST_CHECK(findIt != groupCountMap.end());
            BOOST_CHECK_EQUAL(freqValue, findIt->second);

            // not inserted before
            BOOST_CHECK(checkedSet.insert(group).second);

            // decreasing freq
            BOOST_CHECK_LE(freqValue, maxFreq);
            maxFreq = freqValue;
        }

        // check rest <= maxFreq
        for (GroupCountMap::const_iterator mapIt = groupCountMap.begin();
            mapIt != groupCountMap.end(); ++mapIt)
        {
            if (checkedSet.find(mapIt->first) == checkedSet.end())
            {
                BOOST_CHECK_LE(mapIt->second, maxFreq);
            }
        }
    }

public:
    LogChecker(int queryLimit, int groupLimit)
        : queryDistribution_(1, queryLimit)
        , queryRandom_(engine_, queryDistribution_)
        , groupDistribution_(1, groupLimit)
        , groupRandom_(groupEngine_, groupDistribution_)
        , limitDistribution_(1, groupLimit * 10)
        , limitRandom_(engine_, limitDistribution_)
    {
    }

    void createLog(GroupLabelLogger& logger, int logNum)
    {
        for (int i=0; i<logNum; ++i)
        {
            string query = "query_" + lexical_cast<string>(queryRandom_());
            int group = groupRandom_();

            //cout << "create log: " << query << ", " << group << endl;

            ++queryGroupMap_[query].countMap_[group];
            BOOST_CHECK(logger.logLabel(query, group));
        }
    }

    void setTopLabel(GroupLabelLogger& logger, int topNum)
    {
        for (int i=0; i<topNum; ++i)
        {
            string query = "query_" + lexical_cast<string>(queryRandom_());
            int group = 0;

            // in 50% test, "group" would be empty to reset the top label
            if (i % 2 == 0)
            {
                group = groupRandom_();
            }

            //cout << "set top label: " << query << ", " << group << endl;

            queryGroupMap_[query].manualTop_ = group;
            BOOST_CHECK(logger.setTopLabel(query, group));
        }
    }

    void checkLog(GroupLabelLogger& logger)
    {
        for (QueryGroupMap::const_iterator queryIt = queryGroupMap_.begin();
            queryIt != queryGroupMap_.end(); ++queryIt)
        {
            const string& query = queryIt->first;
            const GroupCounter& groupCounter = queryIt->second;

            int limit = limitRandom_();
            // in 80% test, "limit" value would be 1,
            // as it's used most frequently
            if (limit > limitDistribution_.max() * 0.2)
            {
                limit = 1;
            }
            std::vector<LabelCounter::value_type> valueVec;
            std::vector<int> freqVec;
            BOOST_CHECK(logger.getFreqLabel(query, limit, valueVec, freqVec));

            //cout << "check query: " << query << endl;
            checkGroup_(limit, valueVec, freqVec, groupCounter);
        }
    }
};

}

BOOST_AUTO_TEST_SUITE(GroupLabelLogger_test)

BOOST_AUTO_TEST_CASE(logLabel)
{
    bfs::remove_all(TEST_DIR_STR);
    bfs::create_directory(TEST_DIR_STR);

    LogChecker logChecker(100, 10); // query range, group range
    const int LOG_NUM = 10000; // log number

    {
        GroupLabelLogger logger(TEST_DIR_STR, PROP_NAME_STR);
        BOOST_CHECK(logger.open());

        BOOST_TEST_MESSAGE("check empty log");
        logChecker.checkLog(logger);

        BOOST_TEST_MESSAGE("create log 1st time");
        logChecker.createLog(logger, LOG_NUM);
        logChecker.checkLog(logger);
    }

    {
        GroupLabelLogger logger(TEST_DIR_STR, PROP_NAME_STR);
        BOOST_CHECK(logger.open());

        BOOST_TEST_MESSAGE("check loaded log");
        logChecker.checkLog(logger);

        BOOST_TEST_MESSAGE("create log 2nd time");
        logChecker.createLog(logger, LOG_NUM);
        logChecker.checkLog(logger);
    }
}

BOOST_AUTO_TEST_CASE(setTopLabel)
{
    bfs::remove_all(TEST_DIR_STR);
    bfs::create_directory(TEST_DIR_STR);

    LogChecker logChecker(50, 7); // query range, group range
    const int LOG_NUM = 3000; // log number
    const int TOP_NUM = 500; // set top label number

    {
        GroupLabelLogger logger(TEST_DIR_STR, PROP_NAME_STR);
        BOOST_CHECK(logger.open());

        BOOST_TEST_MESSAGE("set top label");
        logChecker.setTopLabel(logger, TOP_NUM);
        logChecker.checkLog(logger);

        BOOST_TEST_MESSAGE("create log");
        logChecker.createLog(logger, LOG_NUM);
        logChecker.setTopLabel(logger, TOP_NUM);
        logChecker.checkLog(logger);
    }

    {
        GroupLabelLogger logger(TEST_DIR_STR, PROP_NAME_STR);
        BOOST_CHECK(logger.open());

        BOOST_TEST_MESSAGE("check loaded log");
        logChecker.checkLog(logger);

        BOOST_TEST_MESSAGE("create log 2nd time");
        logChecker.setTopLabel(logger, TOP_NUM);
        logChecker.createLog(logger, LOG_NUM);
        logChecker.checkLog(logger);
    }
}

BOOST_AUTO_TEST_SUITE_END() 
