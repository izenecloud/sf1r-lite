/**
 * @file ProductScoreManager.h
 * @brief it manages one ProductScoreTable for each required ProductScoreType.
 * @author Jun Jiang
 * @date Created 2012-11-16
 */

#ifndef SF1R_PRODUCT_SCORE_MANAGER_H
#define SF1R_PRODUCT_SCORE_MANAGER_H

#include <configuration-manager/ProductScoreConfig.h>
#include <map>

namespace sf1r
{
class ProductRankingConfig;
class ProductScorer;
class OfflineProductScorerFactory;
class ProductScoreTable;
class DocumentManager;

class ProductScoreManager
{
public:
    ProductScoreManager(
        const ProductRankingConfig& config,
        OfflineProductScorerFactory& offlineScorerFactory,
        const DocumentManager& documentManager,
        const std::string& dirPath);

    ~ProductScoreManager();

    /**
     * @brief open the @c ProductScoreTables for offline score types.
     * @return true for success, false for failure
     */
    bool open();

    /**
     * @brief build whole offline scores.
     * @return true for success, false for failure
     */
    bool buildCollection();

    /**
     * @return the @c ProductScorer instance which reads @c ProductScoreTable
     *         of @p type, NULL is returned if failed.
     * @attention the caller should delete the instance returned.
     */
    ProductScorer* createProductScorer(ProductScoreType type);

private:
    void createProductScoreTable_(ProductScoreType type);

    bool buildScoreType_(ProductScoreType type);

private:
    const ProductRankingConfig& config_;

    OfflineProductScorerFactory& offlineScorerFactory_;

    const DocumentManager& documentManager_;

    const std::string dirPath_;

    typedef std::map<ProductScoreType, ProductScoreTable*> ScoreTableMap;
    ScoreTableMap scoreTableMap_;
};

} // namespace sf1r

#endif // SF1R_PRODUCT_SCORE_MANAGER_H
