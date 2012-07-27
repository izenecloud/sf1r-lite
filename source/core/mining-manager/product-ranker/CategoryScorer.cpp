#include "CategoryScorer.h"
#include "../group-manager/PropValueTable.h"
#include "../group-label-logger/GroupLabelLogger.h"
#include <search-manager/SearchManager.h>
#include <mining-manager/group-manager/GroupParam.h>
#include <common/NumericPropertyTableBase.h>

#include <glog/logging.h>
#include <boost/scoped_ptr.hpp>
#include <map>

namespace
{
const int kFreqLabelLimit = 100;
const std::string kReasonClickCount("label click count");
const std::string kReasonAvgValue("average value for property ");

using namespace sf1r;

struct LabelValueCounter
{
    double sum_;
    std::size_t count_;

    LabelValueCounter()
        : sum_(0)
        , count_(0)
    {}

    void add(double value)
    {
        sum_ += value;
        ++count_;
    }

    double average() const
    {
        if (count_ != 0)
            return sum_ / count_;

        return 0;
    }
};

typedef std::map<category_id_t, LabelValueCounter> LabelCounterMap;

bool calcLabelAvgValue(
    const LabelCounterMap& counterMap,
    ProductRankingParam::CategoryScores& catScores
)
{
    bool hasAvgValue = false;

    for (LabelCounterMap::const_iterator it = counterMap.begin();
        it != counterMap.end(); ++it)
    {
        const LabelValueCounter& counter = it->second;
        double average = counter.average();

        if (average != 0)
        {
            catScores[it->first] = average;
            hasAvgValue = true;
        }
    }

    return hasAvgValue;
}

} // namespace

namespace sf1r
{

CategoryScorer::CategoryScorer(
        const faceted::PropValueTable* categoryValueTable,
        GroupLabelLogger* categoryClickLogger,
        boost::shared_ptr<SearchManager> searchManager,
        const std::vector<std::string>& avgPropNames)
    : ProductScorer("category")
    , categoryValueTable_(categoryValueTable)
    , categoryClickLogger_(categoryClickLogger)
    , searchManager_(searchManager)
    , avgPropNames_(avgPropNames)
{
}

void CategoryScorer::pushScore(
        const ProductRankingParam& param,
        ProductScoreMatrix& scoreMatrix)
{
    faceted::PropValueTable::ScopedReadLock lock(categoryValueTable_->getMutex());

    for (ProductScoreMatrix::iterator it = scoreMatrix.begin();
        it != scoreMatrix.end(); ++it)
    {
        docid_t docId = it->docId_;
        category_id_t catId = categoryValueTable_->getFirstValueId(docId);
        score_t score = param.getCategoryScore(catId);

        it->pushScore(score);
    }
}

void CategoryScorer::createCategoryScores(ProductRankingParam& param)
{
    if (! getLabelClickCount_(param))
    {
        for (std::vector<std::string>::const_iterator it = avgPropNames_.begin();
            it != avgPropNames_.end(); ++it)
        {
            if (getLabelAvgValue_(*it, param))
                break;
        }
    }

    printScoreReason_(param);
}

void CategoryScorer::printScoreReason_(ProductRankingParam& param) const
{
    if (param.categoryScores_.empty())
    {
        LOG(INFO) << "no category score for query [" << param.query_ << "]";
    }
    else
    {
        LOG(INFO) << param.categoryScores_.size()
                  << " category scores for query [" << param.query_
                  << "], reason: " << param.categoryScoreReason_;
    }
}

bool CategoryScorer::getLabelClickCount_(ProductRankingParam& param)
{
    if (! categoryClickLogger_)
        return false;

    std::vector<faceted::PropValueTable::pvid_t> pvIdVec;
    std::vector<int> freqVec;

    if (categoryClickLogger_->getFreqLabel(
        param.query_, kFreqLabelLimit, pvIdVec, freqVec) &&
        !pvIdVec.empty())
    {
        ProductRankingParam::CategoryScores& catScores = param.categoryScores_;
        const std::size_t catNum = pvIdVec.size();
        const bool hasFreq = freqVec[0] != 0;

        for (std::size_t i = 0; i < catNum; ++i)
        {
            category_id_t catId = pvIdVec[i];
            catScores[catId] = hasFreq ? freqVec[i] : (catNum - i);
        }

        param.categoryScoreReason_ = kReasonClickCount;
        return true;
    }

    return false;
}

bool CategoryScorer::getLabelAvgValue_(
    const std::string& avgPropName,
    ProductRankingParam& param
)
{
    if (!searchManager_)
    {
        LOG(WARNING) << "no SearchManager is found";
        return false;
    }

    boost::shared_ptr<NumericPropertyTableBase>& avgPropTable =
        searchManager_->createPropertyTable(avgPropName);

    if (!avgPropTable)
    {
        LOG(WARNING) << "no NumericPropertyTable is created for property ["
                     << avgPropName << "]";
        return false;
    }

    const std::vector<docid_t>& docIds = param.docIds_;
    faceted::PropValueTable::ScopedReadLock lock(categoryValueTable_->getMutex());
    LabelCounterMap labelCounterMap;

    for (std::vector<docid_t>::const_iterator it = docIds.begin();
        it != docIds.end(); ++it)
    {
        docid_t docId = *it;

        double propValue = 0;
        if (!avgPropTable->getDoubleValue(docId, propValue))
            continue;

        category_id_t catId = categoryValueTable_->getFirstValueId(docId);
        if (catId)
        {
            labelCounterMap[catId].add(propValue);
        }
    }

    if (calcLabelAvgValue(labelCounterMap, param.categoryScores_))
    {
        param.categoryScoreReason_ = kReasonAvgValue + avgPropName;
        return true;
    }

    return false;
}

} // namespace sf1r
