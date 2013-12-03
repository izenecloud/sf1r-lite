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
    limitDistrib_.param(IntDistribT::param_type(1, labelNum * 1.2));
}

void GroupLabelLoggerTestFixture::resetInstance()
{
    BOOST_TEST_MESSAGE(".... reset instance");

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

        //std::cout << "create log: " << query << ", " << labelId << std::endl;

        ++queryLabelCounterFixture_[query].countMap_[labelId];
        BOOST_CHECK(groupLabelLogger_->logLabel(query, labelId));
    }
}

void GroupLabelLoggerTestFixture::setLabel(int setNum)
{
    for (int i = 0; i < setNum; ++i)
    {
        string query = getRandomQuery_();
        std::vector<LabelId> labelIdVec;

        // in 20% test, "labelId" would be empty to reset the top label
        if (i % 5)
        {
            int labelNum = getRandomLimit_();
            for (int j = 0; j < labelNum; ++j)
            {
                labelIdVec.push_back(getRandomLabel_());
            }
        }

        //std::cout << "set top label: " << query
            //<< ", label num: " << labelIdVec.size() << std::endl;

        queryLabelCounterFixture_[query].setLabels_ = labelIdVec;
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
        std::vector<LabelId> labelIdVec;
        std::vector<int> freqVec;

        BOOST_CHECK(groupLabelLogger_->getFreqLabel(query, limit,
                                                    labelIdVec, freqVec));

        //std::cout << "check query: " << query << std::endl;
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
    /*std::cout << "checkLabel_(), limit: " << limit
         << ", result num: " << labelIdVec.size();
    for (std::size_t i = 0; i < labelIdVec.size(); ++i)
    {
        std::cout << ", (" << labelIdVec[i] << ", " << freqVec[i] << ")";
    }
    std::cout << std::endl;*/

    BOOST_CHECK_EQUAL(labelIdVec.size(), freqVec.size());

    if (labelCounterFixture.setLabels_.empty())
    {
        checkLogLabel_(limit, labelIdVec, freqVec, labelCounterFixture.countMap_);
    }
    else
    {
        checkSetLabel_(limit, labelIdVec, freqVec, labelCounterFixture.setLabels_);
    }
}

void GroupLabelLoggerTestFixture::checkLogLabel_(
    int limit,
    const std::vector<LabelId>& labelIdVec,
    const std::vector<int>& freqVec,
    const LabelCountMap& labelCountMap
)
{
    // as the precision of query/label mapping in click log is not
    // high enough, it is disabled, so it should return empty result
    BOOST_CHECK_EQUAL(labelIdVec.size(), 0U);
    BOOST_CHECK_EQUAL(freqVec.size(), 0U);

    /*const int resultNum = labelIdVec.size();
    const int totalNum = labelCountMap.size();
    BOOST_CHECK_EQUAL(resultNum, std::min(totalNum, limit));

    int maxFreq = INT_MAX;
    std::set<LabelId> checkedSet;

    for (int i = 0; i < resultNum; ++i)
    {
        LabelId labelId = labelIdVec[i];
        const int freqValue = freqVec[i];

        // freq value
        LabelCountMap::const_iterator findIt = labelCountMap.find(labelId);
        BOOST_CHECK(findIt != labelCountMap.end());
        BOOST_CHECK_EQUAL(freqValue, findIt->second);

        // not inserted before
        BOOST_CHECK(checkedSet.insert(labelId).second);

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
    }*/
}

void GroupLabelLoggerTestFixture::checkSetLabel_(
    int limit,
    const std::vector<LabelId>& labelIdVec,
    const std::vector<int>& freqVec,
    const std::vector<LabelId>& setLabels
)
{
    const int resultNum = labelIdVec.size();
    const int totalNum = setLabels.size();
    BOOST_CHECK_EQUAL(resultNum, std::min(totalNum, limit));

    BOOST_CHECK_EQUAL_COLLECTIONS(labelIdVec.begin(), labelIdVec.end(),
                                  setLabels.begin(), setLabels.begin()+resultNum);

    std::vector<int> zeroFreqVec(resultNum);
    BOOST_CHECK_EQUAL_COLLECTIONS(freqVec.begin(), freqVec.end(),
                                  zeroFreqVec.begin(), zeroFreqVec.end());
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
