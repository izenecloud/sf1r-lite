///
/// @file MerchantScoreManager.h
/// @brief the class gives all operations on merchant score
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-05-11
///

#ifndef SF1R_MERCHANT_SCORE_MANAGER_H
#define SF1R_MERCHANT_SCORE_MANAGER_H

#include "MerchantScore.h"

#include <string>
#include <vector>
#include <boost/thread.hpp>

namespace sf1r
{

namespace faceted
{
class PropValueTable;
}

class MerchantScoreManager
{
public:
    MerchantScoreManager(
        faceted::PropValueTable* merchantValueTable,
        faceted::PropValueTable* categoryValueTable
    );

    ~MerchantScoreManager();

    bool open(const std::string& scoreFilePath);

    bool flush();

    /**
     * load all existing merchant scores into @p strScoreMap.
     */
    void getAllStrScore(MerchantStrScoreMap& strScoreMap) const;

    /**
     * for each merchant name in @p merchantNames, load its score into @p strScoreMap.
     */
    void getStrScore(
        const std::vector<std::string>& merchantNames,
        MerchantStrScoreMap& strScoreMap
    ) const;

    /**
     * if both @p merchantId and @p categoryId exist, it returns the merchant's category score;
     * if only @p merchantId exists, it returns the merchant's general score;
     * otherwise, it returns zero score.
     */
    score_t getIdScore(
        merchant_id_t merchantId,
        category_id_t categoryId
    ) const;

    /**
     * for each entry in @p strScoreMap, assign its score to @c idScoreMap_.
     */
    void setScore(const MerchantStrScoreMap& strScoreMap);

private:
    bool flushScoreFile_();

    void getCategoryStrScore_(
        const CategoryIdScore& categoryIdScore,
        CategoryStrScore& categoryStrScore
    ) const;

    void setCategoryIdScore_(
        const CategoryStrScore& categoryStrScore,
        CategoryIdScore& categoryIdScore
    );

    merchant_id_t getMerchantId_(const std::string& merchant) const;

    merchant_id_t insertMerchantId_(const std::string& merchant);
    category_id_t insertCategoryId_(const std::string& category);

    void getMerchantName_(merchant_id_t merchantId, std::string& merchant) const;
    void getCategoryName_(category_id_t categoryId, std::string& category) const;

private:
    faceted::PropValueTable* merchantValueTable_;
    faceted::PropValueTable* categoryValueTable_;

    std::string scoreFilePath_;

    MerchantIdScoreMap idScoreMap_;

    typedef boost::shared_mutex MutexType;
    typedef boost::shared_lock<MutexType> ScopedReadLock;
    typedef boost::unique_lock<MutexType> ScopedWriteLock;

    mutable MutexType mutex_;
};

} // namespace sf1r

#endif // SF1R_MERCHANT_SCORE_MANAGER_H
