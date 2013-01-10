/**
 * @file TopKReranker.h
 * @brief rerank topK search results for merchant diversity, etc.
 * @author Jun Jiang
 * @date Created 2013-01-10
 */

#ifndef SF1R_TOPK_RERANKER_H
#define SF1R_TOPK_RERANKER_H

namespace sf1r
{
class SearchManagerPreProcessor;
class ProductRankerFactory;
class KeywordSearchActionItem;
class KeywordSearchResult;

class TopKReranker
{
public:
    TopKReranker(SearchManagerPreProcessor& preprocessor);

    void setProductRankerFactory(ProductRankerFactory* productRankerFactory);

    /**
     * rerank @p resultItem's member @c topKDocs_ and @c topKRankScoreList_.
     */
    bool rerank(
        const KeywordSearchActionItem& actionItem,
        KeywordSearchResult& resultItem);

private:
    SearchManagerPreProcessor& preprocessor_;

    ProductRankerFactory* productRankerFactory_;
};

} // namespace sf1r

#endif // SF1R_TOPK_RERANKER_H
