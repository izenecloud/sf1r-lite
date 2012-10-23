#include "CategoryScorer.h"
#include "../group-manager/PropValueTable.h"
#include "../group-label-logger/GroupLabelLogger.h"
#include <search-manager/SearchManager.h>
#include <common/NumericPropertyTableBase.h>

#include <glog/logging.h>
#include <boost/scoped_ptr.hpp>
#include <map>
#include <sstream>

namespace
{
const int kFreqLabelLimit = 100;

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
    : ProductScorer("click", avgPropNames.size() + 1) // plus click count
    , categoryValueTable_(categoryValueTable)
    , categoryClickLogger_(categoryClickLogger)
    , searchManager_(searchManager)
    , avgPropNames_(avgPropNames)
{
    std::ostringstream oss;
    for (std::vector<std::string>::const_iterator it = avgPropNames_.begin();
        it != avgPropNames_.end(); ++it)
    {
        oss << "\t" << *it;
    }

    scoreMessage_.append(oss.str());
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

        for (int scoreId = 0; scoreId < scoreNum_; ++scoreId)
        {
            score_t score = param.getCategoryScore(scoreId, catId);
            it->pushScore(score);
        }
    }
}

void CategoryScorer::createCategoryScores(ProductRankingParam& param)
{
    param.multiCategoryScores_.resize(scoreNum_);

    getLabelClickCount_(param, 0);

    for (int scoreId = 1; scoreId < scoreNum_; ++scoreId)
    {
        getLabelAvgValue_(avgPropNames_[scoreId-1], param, scoreId);
    }

    printScoreReason_(param);
}

void CategoryScorer::printScoreReason_(const ProductRankingParam& param) const
{
    const ProductRankingParam::MultiCategoryScores& multiScores = param.multiCategoryScores_;
    std::ostringstream oss;
    oss << "(click: " << multiScores[0].size() << ")";

    for (int scoreId = 1; scoreId < scoreNum_; ++scoreId)
    {
        oss << ", (" << avgPropNames_[scoreId-1]
            << ": " << multiScores[scoreId].size() << ")";
    }

    LOG(INFO) << "category score num for query [" << param.query_
              << "]: " << oss.str();
}

bool CategoryScorer::getLabelClickCount_(ProductRankingParam& param, int scoreId)
{
    if (! categoryClickLogger_)
        return false;

    std::vector<faceted::PropValueTable::pvid_t> pvIdVec;
    std::vector<int> freqVec;

    if (categoryClickLogger_->getFreqLabel(
        param.query_, kFreqLabelLimit, pvIdVec, freqVec) &&
        !pvIdVec.empty())
    {
        ProductRankingParam::CategoryScores& catScores =
            param.multiCategoryScores_[scoreId];
        const std::size_t catNum = pvIdVec.size();
        const bool hasFreq = freqVec[0] != 0;

        for (std::size_t i = 0; i < catNum; ++i)
        {
            category_id_t catId = pvIdVec[i];
            catScores[catId] = hasFreq ? freqVec[i] : (catNum - i);
        }

        return true;
    }

    return false;
}

bool CategoryScorer::getLabelAvgValue_(
    const std::string& avgPropName,
    ProductRankingParam& param,
    int scoreId
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
        avgPropTable->getDoubleValue(docId, propValue);

        category_id_t catId = categoryValueTable_->getFirstValueId(docId);
        if (catId)
        {
            labelCounterMap[catId].add(propValue);
        }
    }

    return calcLabelAvgValue(labelCounterMap, param.multiCategoryScores_[scoreId]);
}

} // namespace sf1r
