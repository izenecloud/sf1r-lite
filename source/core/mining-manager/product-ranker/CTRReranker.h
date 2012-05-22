/**
 * @file CTRReranker.h
 * @brief rerank products by CTR count.
 * @author Jun Jiang <jun.jiang@izenesoft.com>
 * @date Created 2012-05-22
 */

#ifndef SF1R_CTR_RERANKER_H
#define SF1R_CTR_RERANKER_H

#include "ProductReranker.h"
#include <boost/shared_ptr.hpp>

namespace sf1r
{
namespace faceted { class CTRManager; }

class CTRReranker : public ProductReranker
{
public:
    CTRReranker(boost::shared_ptr<faceted::CTRManager> ctrManager);

    virtual void rerank(ProductScoreMatrix& scoreMatrix);

private:
    boost::shared_ptr<faceted::CTRManager> ctrManager_;
};

} // namespace sf1r

#endif // SF1R_CTR_RERANKER_H
