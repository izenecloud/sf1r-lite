#include "GroupLabelLoggerTestFixture.h"

#include <set>
#include <algorithm> // min
#include <climits> // INT_MAX

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

namespace
{
const char* TEST_DIR_STR = "group_label_log_test";
const char* PROP_NAME_STR = "CATEGORY";
}

namespace bfs = boost::filesystem;

namespace sf1r
{

GroupLabelLoggerTestFixture::GroupLabelLoggerTestFixture()
{
    bfs::path testPath(TEST_DIR_STR);
    bfs::remove_all(testPath);
    bfs::create_directory(testPath);
}

void GroupLabelLoggerTestFixture::setTestData(int queryNum, int labelNum)
{
    queryDistrib_.param(IntDistribT::param_type(1, queryNum));
    labelDistrib_.param(LabelDistribT::param_type(1, labelNum));
    limitDistrib_.param(IntDistribT::param_type(1, labelNum * 10));
}

void GroupLabelLoggerTestFixture::resetInstance()
{
    BOOST_TEST_MESSAGE("reset instance");

    // flush first
    groupLabelLogger_.reset();
    groupLabelLogger_.reset(new GroupLabelLogger(TEST_DIR_STR, PROP_NAME_STR));
}

void GroupLabelLoggerTestFixture::createLog(int logNum)
{
    for (int i = 0; i < logNum; ++i)
    {
        std::string query = getRandomQuery_();
        LabelId labelId = getRandomLabel_();

        //cout << "create log: " << query << ", " << labelId << endl;

        ++queryLabelCounterFixture_[query].countMap_[labelId];
        BOOST_CHECK(groupLabelLogger_->logLabel(query, labelId));
    }
}

void GroupLabelLoggerTestFixture::setLabel(int setNum)
{
    for (int i = 0; i < setNum; ++i)
    {
        string query = getRandomQuery_();
        LabelId labelId = 0;

        // in 50% test, "labelId" would be empty to reset the top label
        if (i % 2 == 0)
        {
            labelId = getRandomLabel_();
        }

        //cout << "set top label: " << query << ", " << labelId << endl;
        queryLabelCounterFixture_[query].manualTop_ = labelId;

        std::vector<LabelId> labelIdVec;
        if (labelId)
        {
            labelIdVec.push_back(labelId);
        }
        BOOST_CHECK(groupLabelLogger_->setTopLabel(query, labelIdVec));
    }
}

void GroupLabelLoggerTestFixture::checkLabel()
{
    for (QueryLabelCounterFixture::const_iterator queryIt = queryLabelCounterFixture_.begin();
        queryIt != queryLabelCounterFixture_.end(); ++queryIt)
    {
        const string& query = queryIt->first;
        const LabelCounterFixture& labelCounterFixture = queryIt->second;

        int limit = getRandomLimit_();
        // in 80% test, "limit" value would be 1,
        // as it's used most frequently
        if (limit > limitDistrib_.max() * 0.2)
        {
            limit = 1;
        }
        std::vector<LabelId> labelIdVec;
        std::vector<int> freqVec;
        BOOST_CHECK(groupLabelLogger_->getFreqLabel(query, limit, labelIdVec, freqVec));

        //cout << "check query: " << query << endl;
        checkLabel_(limit, labelIdVec, freqVec, labelCounterFixture);
    }
}

void GroupLabelLoggerTestFixture::checkLabel_(
    int limit,
    const std::vector<LabelId>& labelIdVec,
    const std::vector<int>& freqVec,
    const LabelCounterFixture& labelCounterFixture
)
{
    //cout << "checkLabel_(), limit: " << limit
            //<< ", labelIdVec.size(): " << labelIdVec.size();
    //for (size_t i=0; i<labelIdVec.size(); ++i)
    //{
        //cout << ", (" << labelIdVec[i] << ", " << freqVec[i] << ")";
    //}
    //cout << endl;


    BOOST_CHECK_EQUAL(labelIdVec.size(), freqVec.size());
    BOOST_CHECK_LE(static_cast<int>(labelIdVec.size()), limit);

    std::set<LabelId> checkedSet;
    size_t i = 0;
    const LabelCountMap& labelCountMap = labelCounterFixture.countMap_;
    int totalCount = labelCountMap.size();

    // check top label set manually
    if (labelCounterFixture.manualTop_)
    {
        LabelId labelId = labelCounterFixture.manualTop_;
        BOOST_CHECK_EQUAL(labelIdVec[0], labelId);
        BOOST_CHECK_EQUAL(freqVec[0], 0);
        BOOST_CHECK_EQUAL(labelIdVec.size(), 1U);

        return;
    }

    // check result size
    BOOST_CHECK_EQUAL(static_cast<int>(labelIdVec.size()),
                      std::min(totalCount, limit));

    int maxFreq = INT_MAX;
    for (; i < labelIdVec.size(); ++i)
    {
        LabelId group = labelIdVec[i];
        const int freqValue = freqVec[i];

        // freq value
        LabelCountMap::const_iterator findIt = labelCountMap.find(group);
        BOOST_CHECK(findIt != labelCountMap.end());
        BOOST_CHECK_EQUAL(freqValue, findIt->second);

        // not inserted before
        BOOST_CHECK(checkedSet.insert(group).second);

        // decreasing freq
        BOOST_CHECK_LE(freqValue, maxFreq);
        maxFreq = freqValue;
    }

    // check rest <= maxFreq
    for (LabelCountMap::const_iterator mapIt = labelCountMap.begin();
        mapIt != labelCountMap.end(); ++mapIt)
    {
        if (checkedSet.find(mapIt->first) == checkedSet.end())
        {
            BOOST_CHECK_LE(mapIt->second, maxFreq);
        }
    }
}

std::string GroupLabelLoggerTestFixture::getRandomQuery_()
{
    int random = queryDistrib_(queryEngine_);
    std::string str = boost::lexical_cast<std::string>(random);
    return "query_" + str;
}

GroupLabelLoggerTestFixture::LabelId GroupLabelLoggerTestFixture::getRandomLabel_()
{
    return labelDistrib_(labelEngine_);
}

int GroupLabelLoggerTestFixture::getRandomLimit_()
{
    return limitDistrib_(limitEngine_);
}

} // namespace sf1r
