#include "CTRReranker.h"
#include "../faceted-submanager/ctr_manager.h"

#include <algorithm> // stable_sort

namespace sf1r
{

CTRReranker::CTRReranker(boost::shared_ptr<faceted::CTRManager> ctrManager)
    : ctrManager_(ctrManager)
{
}

void CTRReranker::rerank(ProductScoreMatrix& scoreMatrix)
{
    faceted::CTRManager::ScopedReadLock lock(ctrManager_->getMutex());

    for (ProductScoreMatrix::iterator it = scoreMatrix.begin();
        it != scoreMatrix.end(); ++it)
    {
        docid_t docId = it->docId_;

        it->ctrCount_ = ctrManager_->getClickCountByDocId(docId);
    }

    std::stable_sort(scoreMatrix.begin(), scoreMatrix.end(), compareProductScoreListByCTR);
}

} // namespace sf1r
