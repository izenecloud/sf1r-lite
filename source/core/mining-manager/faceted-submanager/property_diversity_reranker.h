/**
 * @file property_diversity_reranker.h
 * @author Yingfeng Zhang
 * @date Jun 28, 2011
 * @brief Providing below reranking methods:
 * - boost with top clicked label
 * - diversity rerank
 * - "Click Through Rate" rerank
 */
#ifndef PROPERTY_DIVERSITY_RERANKER_H
#define PROPERTY_DIVERSITY_RERANKER_H

#include <boost/scoped_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/weak_ptr.hpp>

#include "group_manager.h"
#include "ctr_manager.h"

#include <common/inttypes.h>

namespace sf1r
{
class GroupLabelLogger;
class SearchManager;
}

NS_FACETED_BEGIN

struct LabelValueCounter
{
    LabelValueCounter()
    : totalvalue_(0)
    , avgValue_(0)
    , cnt_(0)
    {}

    void compute()
    {
        if (cnt_ > 0)
        {
            avgValue_ = totalvalue_ / cnt_;
        }
    }

    double totalvalue_;
    double avgValue_;
    std::size_t cnt_;
};

class GroupManager;

class PropertyDiversityReranker
{
public:
    PropertyDiversityReranker(
        const GroupManager* groupManager,
        const std::string& diversityProperty,
        const std::string& boostingProperty
    );

    ~PropertyDiversityReranker();

    void setBoostingPolicyProperty(const std::string& property)
    {
        boostingPolicyProperty_ = property;
    }

    void setGroupLabelLogger(GroupLabelLogger* logger)
    {
        groupLabelLogger_ = logger;
    }

    void setCTRManager(boost::shared_ptr<CTRManager> ctrManager)
    {
        ctrManager_ = ctrManager;
    }

    void setSearchManager(boost::shared_ptr<SearchManager> searchManager)
    {
        searchManager_ = searchManager;
    }

    /**
     * 1st, it boosts the top label,
     * 2nd, it reranks with "property diversity",
     * 3rd, it reranks with "Click Through Rate".
     */
    void rerank(
        std::vector<unsigned int>& docIdList,
        std::vector<float>& rankScoreList,
        const std::string& query
    );

private:
    /**
     * get first level label for specified pvid
     * @param parentIdList
     * @param pvId
     */
    PropValueTable::pvid_t getFirstLevelLabel(
            const PropValueTable::ValueIdList& parentIdList,
            PropValueTable::pvid_t pvId);

    /**
     * Get boosting label id by policy
     * @return label id
     */
    PropValueTable::pvid_t getBoostLabelIdByPolicy_(
            std::vector<unsigned int>& docIdList);

    /**
     * implementation for @c rerank() using below rerank methods:
     * - rerank with "property diversity"
     * - rerank with "Click Through Rate"
     */
    void rerankImpl_(
        std::vector<unsigned int>& docIdList,
        std::vector<float>& rankScoreList
    );

    /**
     * rerank with "property diversity"
     */
    void rerankDiversity_(
        std::vector<unsigned int>& docIdList,
        std::vector<float>& rankScoreList
    );

    /**
     * rerank with "Click Through Rate"
     */
    void rerankCTR_(
        std::vector<unsigned int>& docIdList,
        std::vector<float>& rankScoreList
    );

private:
    const GroupManager* groupManager_;
    std::string diversityProperty_;
    std::string boostingProperty_;
    std::string boostingPolicyProperty_;

    GroupLabelLogger* groupLabelLogger_;
    boost::weak_ptr<SearchManager> searchManager_;
    boost::shared_ptr<CTRManager> ctrManager_;
};


NS_FACETED_END


#endif /* PROPERTY_DIVERSITY_RERANKER_H */

