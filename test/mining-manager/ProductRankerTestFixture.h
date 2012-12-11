/**
 * @file ProductRankerTestFixture.h
 * @brief fixture class to test ProductRanker instance.
 * @author Jun Jiang
 * @date Created 2012-11-23
 */

#ifndef SF1R_PRODUCT_RANKER_TEST_FIXTURE_H
#define SF1R_PRODUCT_RANKER_TEST_FIXTURE_H

#include <configuration-manager/ProductRankingConfig.h>
#include <mining-manager/group-manager/PropValueTable.h>
#include <string>
#include <vector>

namespace sf1r
{

class ProductRankerTestFixture
{
public:
    ProductRankerTestFixture();

    void configRandomScore();

    void setDocId(const std::string& docIdList);
    void setTopKScore(const std::string& scoreList);

    void setMultiDocMerchantId(const std::string& merchantList);
    void setSingleDocMerchantId(const std::string& merchantList);

    void rank();

    void checkDocId(const std::string& goldDocIdList);
    void checkTopKScore(const std::string& goldScoreList);

    void checkDocId(
        const std::string& goldDocIdList,
        int equalNum);

    void checkTopKScore(
        const std::string& goldScoreList,
        int equalNum);

protected:
    ProductRankingConfig rankConfig_;
    faceted::PropValueTable merchantValueTable_;

    std::vector<docid_t> docIds_;
    std::vector<score_t> topKScores_;
};

} // namespace sf1r

#endif // SF1R_PRODUCT_RANKER_TEST_FIXTURE_H
