#include "MerchantScorer.h"
#include "../faceted-submanager/prop_value_table.h"
#include "../merchant-score-manager/MerchantScoreManager.h"

namespace
{
using namespace sf1r;

category_id_t getCategoryId(
    const faceted::PropValueTable::ValueIdTable* categoryIdTable,
    docid_t docId
)
{
    if (categoryIdTable && docId < categoryIdTable->size())
    {
        const faceted::PropValueTable::ValueIdList& valueIdList = (*categoryIdTable)[docId];

        if (!valueIdList.empty())
            return valueIdList[0];
    }

    return 0;
}

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

        if (merchantCount != 0)
        {
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

    }

    scoreList.pushScore(merchantCount);
    scoreList.pushScore(merchantScore);

}

}

namespace sf1r
{

MerchantScorer::MerchantScorer(
    const faceted::PropValueTable* categoryValueTable,
    const faceted::PropValueTable* merchantValueTable,
    const MerchantScoreManager* merchantScoreManager
)
    : categoryValueTable_(categoryValueTable)
    , merchantValueTable_(merchantValueTable)
    , merchantScoreManager_(merchantScoreManager)
{
}

void MerchantScorer::pushScore(
    const ProductRankingParam& param,
    ProductScoreMatrix& scoreMatrix
)
{
    faceted::PropValueTable::ScopedReadLock merchantLock(merchantValueTable_->getLock());
    const faceted::PropValueTable::ValueIdTable& merchantIdTable = merchantValueTable_->valueIdTable();

    faceted::PropValueTable::LockType* categoryLock = NULL;
    const faceted::PropValueTable::ValueIdTable* categoryIdTable = NULL;

    if (categoryValueTable_)
    {
        categoryIdTable = &categoryValueTable_->valueIdTable();
        categoryLock = &categoryValueTable_->getLock();
        categoryLock->lock_shared();
    }

    for (ProductScoreMatrix::iterator it = scoreMatrix.begin();
        it != scoreMatrix.end(); ++it)
    {
        ProductScoreList& scoreList = *it;
        docid_t docId = scoreList.docId_;

        category_id_t categoryId = getCategoryId(categoryIdTable, docId);
        categoryId = merchantValueTable_->getRootValueId(categoryId);

        getMerchantCountScore(merchantIdTable, merchantScoreManager_,
            docId, categoryId, scoreList);
    }

    if (categoryLock)
    {
        categoryLock->unlock_shared();
    }
}

} // namespace sf1r
