///
/// @file GroupLabelLoggerTestFixture.h
/// @brief fixture class to test GroupLabelLogger functions.
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-06-20
///

#ifndef SF1R_GROUP_LABEL_LOGGER_TEST_FIXTURE_H
#define SF1R_GROUP_LABEL_LOGGER_TEST_FIXTURE_H

#include <mining-manager/group-label-logger/GroupLabelLogger.h>

#include <map>
#include <vector>
#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

namespace sf1r
{

class GroupLabelLoggerTestFixture
{
public:
    GroupLabelLoggerTestFixture();

    void setTestData(int queryNum, int labelNum);

    void resetInstance();

    void createLog(int logNum);

    void setLabel(int setNum);

    void checkLabel();

private:
    typedef LabelCounter::LabelId LabelId;
    typedef std::map<LabelId, int> LabelCountMap;

    struct LabelCounterFixture
    {
        LabelCountMap countMap_;
        std::vector<LabelId> setLabels_;
    };

    void checkLabel_(
        int limit,
        const std::vector<LabelId>& labelIdVec,
        const std::vector<int>& freqVec,
        const LabelCounterFixture& labelCounterFixture
    );

    void checkLogLabel_(
        int limit,
        const std::vector<LabelId>& labelIdVec,
        const std::vector<int>& freqVec,
        const LabelCountMap& labelCountMap
    );

    void checkSetLabel_(
        int limit,
        const std::vector<LabelId>& labelIdVec,
        const std::vector<int>& freqVec,
        const std::vector<LabelId>& setLabels
    );

    std::string getRandomQuery_();
    LabelId getRandomLabel_();
    int getRandomLimit_();

private:
    typedef boost::random::uniform_int_distribution<> IntDistribT;
    typedef boost::random::uniform_int_distribution<LabelId> LabelDistribT;

    // param "query" of GroupLabelLogger::logLabel()
    boost::random::mt19937 queryEngine_;
    IntDistribT queryDistrib_;

    // param "labelId" of GroupLabelLogger::logLabel()
    boost::random::mt11213b labelEngine_;
    LabelDistribT labelDistrib_;

    // param "limit" of GroupLabelLogger::getFreqLabel()
    boost::random::mt19937 limitEngine_;
    IntDistribT limitDistrib_;

    typedef std::map<std::string, LabelCounterFixture> QueryLabelCounterFixture;
    QueryLabelCounterFixture queryLabelCounterFixture_;

    boost::scoped_ptr<GroupLabelLogger> groupLabelLogger_;
};

} // namespace sf1r

#endif // SF1R_GROUP_LABEL_LOGGER_TEST_FIXTURE_H
