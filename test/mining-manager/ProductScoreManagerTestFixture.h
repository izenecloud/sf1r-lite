/**
 * @file ProductScoreManagerTestFixture.h
 * @brief fixture class to test ProductScoreManager functions.
 * @author Jun Jiang
 * @date Created 2012-11-19
 */

#ifndef SF1R_PRODUCT_SCORE_MANAGER_TEST_FIXTURE_H
#define SF1R_PRODUCT_SCORE_MANAGER_TEST_FIXTURE_H

#include <configuration-manager/PropertyConfig.h>
#include <configuration-manager/ProductRankingConfig.h>
#include <configuration-manager/MiningConfig.h>
#include <string>
#include <vector>

namespace sf1r
{
class OfflineProductScorerFactory;
class ProductScoreManager;
class DocumentManager;
class ProductScorer;

class ProductScoreManagerTestFixture
{
public:
    ProductScoreManagerTestFixture();
    ~ProductScoreManagerTestFixture();

    void resetScoreManager();

    void buildScore(int docNum);

    void insertDocument(int docNum);
    void buildCollection();
    void updateGoldScore();

    void checkScore();
    void readScore(int docNum);

private:
    void initConfig_();

protected:
    OfflineProductScorerFactory* offlineScorerFactory_;
    ProductScoreManager* productScoreManager_;

    IndexBundleSchema schema_;
    DocumentManager* documentManager_;

    std::string scoreDirPath_;
    ProductRankingConfig rankConfig_;
    ProductScoreConfig& testScoreConfig_;
    ProductRankingPara bundleParam_;

    docid_t lastDocId_;
    std::vector<score_t> goldScores_;

    OfflineProductScorerFactory* goldScorerFactory_;
    ProductScorer* goldScorer_;
};

} // namespace sf1r

#endif // SF1R_PRODUCT_SCORE_MANAGER_TEST_FIXTURE_H
