#include "CategoryBoostingScorer.h"
#include "../group-manager/PropValueTable.h"
#include "../group-label-logger/GroupLabelLogger.h"
#include <search-manager/SearchManager.h>
#include <mining-manager/group-manager/GroupParam.h>

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
    , boostingReasons_(BOOSTING_REASON_NUM)
{
    boostingReasons_[BOOSTING_SELECT_LABEL] = "the 1st selected label";
    boostingReasons_[BOOSTING_FREQ_LABEL] = "most frequent click";
    boostingReasons_[BOOSTING_MAX_AVG_LABEL] = "max avg value for property [" +
                                               boostingSubProp_ + "]";
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
    category_id_t boostLabel = 0;
    BOOSTING_REASON reason = BOOSTING_REASON_NUM;

    if ((boostLabel = getFirstSelectLabel_(param.groupParam_)))
    {
        reason = BOOSTING_SELECT_LABEL;
    }
    else if ((boostLabel = getFreqLabel_(param.query_)))
    {
        reason = BOOSTING_FREQ_LABEL;
    }
    else if (!boostingSubProp_.empty() &&
             (boostLabel = getMaxAvgLabel_(param.docIds_)))
    {
        reason = BOOSTING_MAX_AVG_LABEL;
    }

    param.boostingCategoryId_ = boostLabel;
    printBoostingLabel_(param.query_, boostLabel, reason);
}

void CategoryBoostingScorer::printBoostingLabel_(
    const std::string& query,
    category_id_t boostLabel,
    BOOSTING_REASON reason
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
              << "], reason: " << boostingReasons_[reason];
}

category_id_t CategoryBoostingScorer::getFirstSelectLabel_(const faceted::GroupParam& groupParam)
{
    const faceted::GroupParam::GroupLabelMap& labelMap = groupParam.groupLabels_;
    const std::string& propName = categoryValueTable_->propName();
    faceted::GroupParam::GroupLabelMap::const_iterator findIt = labelMap.find(propName);

    if (findIt == labelMap.end())
        return 0;

    const faceted::GroupParam::GroupPathVec& pathVec = findIt->second;
    if (pathVec.size() <= 1)
        return 0;

    const faceted::GroupParam::GroupPath& path = pathVec.front();
    std::vector<izenelib::util::UString> ustrPath;

    for (std::vector<std::string>::const_iterator nodeIt = path.begin();
        nodeIt != path.end(); ++nodeIt)
    {
        ustrPath.push_back(izenelib::util::UString(*nodeIt,
                           izenelib::util::UString::UTF_8));
    }

    return categoryValueTable_->propValueId(ustrPath);
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
