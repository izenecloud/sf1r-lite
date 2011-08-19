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

#include "ctr_manager.h"

#include <common/inttypes.h>

namespace sf1r
{
class GroupLabelLogger;
}

NS_FACETED_BEGIN

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

    void setGroupLabelLogger(GroupLabelLogger* logger)
    {
        groupLabelLogger_ = logger;
    }

    void setCTRManager(boost::shared_ptr<CTRManager> ctrManager)
    {
        ctrManager_ = ctrManager;
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

    GroupLabelLogger* groupLabelLogger_;

    boost::shared_ptr<CTRManager> ctrManager_;
};


NS_FACETED_END


#endif /* PROPERTY_DIVERSITY_RERANKER_H */

