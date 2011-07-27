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

    typedef map<string, int> GroupCountMap;
    typedef map<string, GroupCountMap> QueryGroupMap;
    QueryGroupMap queryGroupMap_;

private:
    void checkGroup_(
        int limit,
        const std::vector<std::string>& propValueVec,
        const std::vector<int>& freqVec,
        const GroupCountMap& groupCountMap
    )
    {
        //cout << "checkGroup_(), limit: " << limit
             //<< ", propValueVec.size(): " << propValueVec.size();
        //for (size_t i=0; i<propValueVec.size(); ++i)
        //{
            //cout << ", (" << propValueVec[i] << ", " << freqVec[i] << ")";
        //}
        //cout << endl;

        BOOST_CHECK_EQUAL(propValueVec.size(), freqVec.size());
        BOOST_CHECK_LE(propValueVec.size(), limit);
        BOOST_CHECK_GT(propValueVec.size(), 0);

        set<string> checkedSet;
        int maxFreq = INT_MAX;
        for (size_t i=0; i<propValueVec.size(); ++i)
        {
            const string& groupStr = propValueVec[i];
            const int freqValue = freqVec[i];

            // freq value
            GroupCountMap::const_iterator findIt = groupCountMap.find(groupStr);
            BOOST_CHECK(findIt != groupCountMap.end());
            BOOST_CHECK_EQUAL(freqValue, findIt->second);

            // not inserted before
            BOOST_CHECK(checkedSet.insert(groupStr).second);

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
        string query;
        string group;

        for (int i=0; i<logNum; ++i)
        {
            query = "query_" + lexical_cast<string>(queryRandom_());
            group = "group_" + lexical_cast<string>(groupRandom_());

            //cout << "create log: " << query << ", " << group << endl;

            ++queryGroupMap_[query][group];
            BOOST_CHECK(logger.logLabel(query, group));
        }
    }

    void checkLog(GroupLabelLogger& logger)
    {
        for (QueryGroupMap::const_iterator queryIt = queryGroupMap_.begin();
            queryIt != queryGroupMap_.end(); ++queryIt)
        {
            const string& query = queryIt->first;
            const GroupCountMap& groupCountMap = queryIt->second;

            int limit = limitRandom_();
            // in 80% test, "limit" value would be 1,
            // as it's used most frequently
            if (limit > limitDistribution_.max() * 0.2)
            {
                limit = 1;
            }
            std::vector<std::string> propValueVec;
            std::vector<int> freqVec;
            BOOST_CHECK(logger.getFreqLabel(query, limit, propValueVec, freqVec));

            //cout << "check query: " << query << endl;
            checkGroup_(limit, propValueVec, freqVec, groupCountMap);
        }
    }
};

}

BOOST_AUTO_TEST_SUITE(GroupLabelLogger_test)

BOOST_AUTO_TEST_CASE(getFreqLabel)
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

BOOST_AUTO_TEST_SUITE_END() 
