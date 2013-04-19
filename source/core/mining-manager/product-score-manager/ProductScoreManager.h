/**
 * @file ProductScoreManager.h
 * @brief it manages one ProductScoreTable for each required ProductScoreType.
 * @author Jun Jiang
 * @date Created 2012-11-16
 */

#ifndef SF1R_PRODUCT_SCORE_MANAGER_H
#define SF1R_PRODUCT_SCORE_MANAGER_H

#include <configuration-manager/ProductScoreConfig.h>
#include <util/cronexpression.h>
#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>
#include <map>
#include <string>

namespace sf1r
{
class ProductRankingConfig;
class ProductScorer;
class OfflineProductScorerFactory;
class ProductScoreTable;
class DocumentManager;
class ProductRankingPara;
class SearchCache;

class ProductScoreManager
{
public:
    ProductScoreManager(
        const ProductRankingConfig& config,
        const ProductRankingPara& bundleParam,
        OfflineProductScorerFactory& offlineScorerFactory,
        const DocumentManager& documentManager,
        const std::string& collectionName,
        const std::string& dirPath,
        boost::shared_ptr<SearchCache> searchCache);

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
     *         corresponding to @p config, NULL is returned if failed.
     * @attention the caller should delete the instance returned.
     */
    ProductScorer* createProductScorer(const ProductScoreConfig& config) const;

    /**
     * @return the @c ProductScoreTable instance of @p type,
     *         NULL is returned if failed.
     */
    const ProductScoreTable* getProductScoreTable(ProductScoreType type) const;

private:
    void createProductScoreTable_(ProductScoreType type);
    bool isEmpty_() const;

    bool buildScoreType_(ProductScoreType type);

    bool addCronJob_(const ProductRankingPara& bundleParam);
    void runCronJob_(int calltype);

    void clearSearchCache_();

private:
    const ProductRankingConfig& config_;

    OfflineProductScorerFactory& offlineScorerFactory_;

    const DocumentManager& documentManager_;

    const std::string dirPath_;

    typedef std::map<ProductScoreType, ProductScoreTable*> ScoreTableMap;
    ScoreTableMap scoreTableMap_;

    izenelib::util::CronExpression cronExpression_;
    const std::string cronJobName_;
    boost::mutex buildCollectionMutex_;

    boost::shared_ptr<SearchCache> searchCache_;
};

} // namespace sf1r

#endif // SF1R_PRODUCT_SCORE_MANAGER_H
