#include "CategoryBoostingScorer.h"
#include "../group-manager/PropValueTable.h"
#include "../group-label-logger/GroupLabelLogger.h"
#include <search-manager/SearchManager.h>

#include <glog/logging.h>
#include <boost/scoped_ptr.hpp>
#include <map>

namespace
{
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

typedef std::map<faceted::PropValueTable::pvid_t, LabelValueCounter> LabelCounterMap;

faceted::PropValueTable::pvid_t getMaxAvgLabelId(LabelCounterMap& counterMap)
{
    faceted::PropValueTable::pvid_t maxLabelId = 0;
    double maxAvg = 0;

    for (LabelCounterMap::iterator it = counterMap.begin();
        it != counterMap.end(); ++it)
    {
        LabelValueCounter& counter = it->second;
        double average = counter.average();

        if (average > maxAvg)
        {
            maxLabelId = it->first;
            maxAvg = average;
        }
    }

    return maxLabelId;
}

} // namespace

namespace sf1r
{

CategoryBoostingScorer::CategoryBoostingScorer(
    const faceted::PropValueTable* categoryValueTable,
    GroupLabelLogger* categoryClickLogger,
    boost::shared_ptr<SearchManager> searchManager,
    const std::string& boostingSubProp
)
    : ProductScorer("boost")
    , categoryValueTable_(categoryValueTable)
    , categoryClickLogger_(categoryClickLogger)
    , searchManager_(searchManager)
    , boostingSubProp_(boostingSubProp)
    , reasonFreqLabel_("most frequent click")
    , reasonMaxAvgLabel_("max avg value for property [" + boostingSubProp_ + "]")
{
}

void CategoryBoostingScorer::pushScore(
    const ProductRankingParam& param,
    ProductScoreMatrix& scoreMatrix
)
{
    category_id_t boostingCategory = param.boostingCategoryId_;
    faceted::PropValueTable::ScopedReadLock lock(categoryValueTable_->getMutex());

    for (ProductScoreMatrix::iterator it = scoreMatrix.begin();
        it != scoreMatrix.end(); ++it)
    {
        score_t boostingScore = 0;
        docid_t docId = it->docId_;

        if (boostingCategory &&
            categoryValueTable_->testDoc(docId, boostingCategory))
        {
            boostingScore = 1;
        }

        it->pushScore(boostingScore);
    }
}

void CategoryBoostingScorer::selectBoostingCategory(ProductRankingParam& param)
{
    category_id_t boostLabel = getFreqLabel_(param.query_);
    bool isMaxAvgLabel = false;

    if (boostLabel == 0 && !boostingSubProp_.empty())
    {
        boostLabel = getMaxAvgLabel_(param.docIds_);
        isMaxAvgLabel = true;
    }

    param.boostingCategoryId_ = boostLabel;
    printBoostingLabel_(param.query_, boostLabel, isMaxAvgLabel);
}

void CategoryBoostingScorer::printBoostingLabel_(
    const std::string& query,
    category_id_t boostLabel,
    bool isMaxAvgLabel
) const
{
    if (boostLabel == 0)
    {
        LOG(INFO) << "no boosting category is selected for query [" << query << "]";
        return;
    }

    izenelib::util::UString labelUStr;
    categoryValueTable_->propValueStr(boostLabel, labelUStr);

    std::string labelStr;
    labelUStr.convertString(labelStr, izenelib::util::UString::UTF_8);

    LOG(INFO) << "boosting category [" << labelStr << "] for query [" << query
              << "], reason: " << (isMaxAvgLabel ? reasonMaxAvgLabel_ : reasonFreqLabel_);
}

category_id_t CategoryBoostingScorer::getFreqLabel_(const std::string& query)
{
    if (! categoryClickLogger_)
        return 0;

    std::vector<faceted::PropValueTable::pvid_t> pvIdVec;
    std::vector<int> freqVec;

    if (categoryClickLogger_->getFreqLabel(query, 1, pvIdVec, freqVec) &&
        !pvIdVec.empty())
    {
        return pvIdVec.front();
    }

    return 0;
}

category_id_t CategoryBoostingScorer::getMaxAvgLabel_(const std::vector<docid_t>& docIds)
{
    boost::scoped_ptr<NumericPropertyTable> subPropTable;

    if (searchManager_)
    {
        subPropTable.reset(searchManager_->createPropertyTable(boostingSubProp_));
    }

    if (! subPropTable)
    {
        LOG(WARNING) << "no NumericPropertyTable is created for property [" << boostingSubProp_ << "]";
        return 0;
    }

    faceted::PropValueTable::ScopedReadLock lock(categoryValueTable_->getMutex());
    LabelCounterMap labelCounterMap;

    for (std::vector<docid_t>::const_iterator it = docIds.begin();
        it != docIds.end(); ++it)
    {
        docid_t docId = *it;

        double subPropValue = 0;
        if (! subPropTable->convertPropertyValue(docId, subPropValue))
            continue;

        faceted::PropValueTable::pvid_t pvId = categoryValueTable_->getRootValueId(docId);
        if (pvId)
        {
            labelCounterMap[pvId].add(subPropValue);
        }
    }

    return getMaxAvgLabelId(labelCounterMap);
}

} // namespace sf1r
