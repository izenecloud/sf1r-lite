#include "MerchantScorer.h"
#include "../group-manager/PropValueTable.h"
#include "../merchant-score-manager/MerchantScoreManager.h"

namespace
{
using namespace sf1r;

class AllowEmptyLock
{
public:
    AllowEmptyLock(faceted::PropValueTable::MutexType* mutex)
        : mutex_(mutex)
    {
        if (mutex_)
        {
            mutex_->lock_shared();
        }
    }

    ~AllowEmptyLock()
    {
        if (mutex_)
        {
            mutex_->unlock_shared();
        }
    }

private:
    faceted::PropValueTable::MutexType* mutex_;
};

score_t sumMerchantScore(
    const MerchantScoreManager* merchantScoreManager,
    const faceted::PropValueTable::ValueIdList& merchantIdList,
    category_id_t categoryId
)
{
    score_t sum = 0;

    for (faceted::PropValueTable::ValueIdList::const_iterator it = merchantIdList.begin();
        it != merchantIdList.end(); ++it)
    {
        sum += merchantScoreManager->getIdScore(*it, categoryId);
    }

    return sum;
}

void getMerchantCountScore(
    const faceted::PropValueTable::ValueIdTable& merchantIdTable,
    const MerchantScoreManager* merchantScoreManager,
    docid_t docId,
    category_id_t categoryId,
    ProductScoreList& scoreList
)
{
    score_t merchantCount = 0;
    score_t merchantScore = 0;

    if (docId < merchantIdTable.size())
    {
        const faceted::PropValueTable::ValueIdList& valueIdList = merchantIdTable[docId];
        merchantCount = valueIdList.size();

        if (merchantCount == 1.0)
        {
            scoreList.singleMerchantId_ = valueIdList.front();
        }
        else if (merchantCount > 2)
        {
            // for multiple merchants, their "merchantCount" are all set to 2,
            // so that they could be compared by their following scores
            merchantCount = 2;
        }

        merchantScore = sumMerchantScore(merchantScoreManager, valueIdList, categoryId);
    }

    scoreList.pushScore(merchantCount);
    scoreList.pushScore(merchantScore);
}

} // namespace

namespace sf1r
{

MerchantScorer::MerchantScorer(
    const faceted::PropValueTable* categoryValueTable,
    const faceted::PropValueTable* merchantValueTable,
    const MerchantScoreManager* merchantScoreManager
)
    : ProductScorer("mcount\tmscore")
    , categoryValueTable_(categoryValueTable)
    , merchantValueTable_(merchantValueTable)
    , merchantScoreManager_(merchantScoreManager)
{
}

void MerchantScorer::pushScore(
    const ProductRankingParam& param,
    ProductScoreMatrix& scoreMatrix
)
{
    faceted::PropValueTable::ScopedReadLock merchantLock(merchantValueTable_->getMutex());
    const faceted::PropValueTable::ValueIdTable& merchantIdTable = merchantValueTable_->valueIdTable();

    faceted::PropValueTable::MutexType* categoryMutex =
        categoryValueTable_ ? &categoryValueTable_->getMutex() : NULL;
    AllowEmptyLock categoryLock(categoryMutex);

    for (ProductScoreMatrix::iterator it = scoreMatrix.begin();
        it != scoreMatrix.end(); ++it)
    {
        ProductScoreList& scoreList = *it;
        docid_t docId = scoreList.docId_;

        category_id_t categoryId = 0;
        if (categoryValueTable_)
        {
            categoryId = categoryValueTable_->getRootValueId(docId);
        }

        getMerchantCountScore(merchantIdTable, merchantScoreManager_,
            docId, categoryId, scoreList);
    }
}

} // namespace sf1r
