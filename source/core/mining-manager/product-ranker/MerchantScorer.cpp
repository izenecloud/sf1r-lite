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

void getMerchantCountScore(
    const faceted::PropValueTable* merchantValueTable,
    const MerchantScoreManager* merchantScoreManager,
    docid_t docId,
    category_id_t categoryId,
    ProductScoreList& scoreList
)
{
    faceted::PropValueTable::PropIdList merchantIdList;
    merchantValueTable->getPropIdList(docId, merchantIdList);

    score_t merchantCount = merchantIdList.size();
    if (merchantCount == 1.0)
    {
        scoreList.singleMerchantId_ = merchantIdList[0];
    }
    else if (merchantCount > 2)
    {
        // for multiple merchants, their "merchantCount" are all set to 2,
        // so that they could be compared by their following scores
        merchantCount = 2;
    }

    score_t merchantScore = 0;
    for (std::size_t i = 0; i < merchantIdList.size(); ++i)
    {
        merchantScore += merchantScoreManager->getIdScore(
            merchantIdList[i], categoryId);
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

        getMerchantCountScore(merchantValueTable_, merchantScoreManager_,
            docId, categoryId, scoreList);
    }
}

} // namespace sf1r
