/**
 * @file BoostLabelSelector.h
 * @brief It selects the labels to boost the product rankings.
 *
 * The labels are selected according to below priority (from high to low):
 * 1. the lables clicked most frequently, updated by SF1 API
 *    set_top_group_label() and log_group_label().
 * 2. the label classified from query by @c MiningManager::GetProductCategory().
 * 3. the knowledge label from @c GroupLabelKnowledge::getKnowledgeLabel().
 *
 * @author Jun Jiang
 * @date Created 2012-12-22
 */

#ifndef SF1R_BOOST_LABEL_SELECTOR_H
#define SF1R_BOOST_LABEL_SELECTOR_H

#include <common/inttypes.h>
#include <vector>
#include <string>

namespace sf1r
{
class MiningManager;
class GroupLabelLogger;
class GroupLabelKnowledge;
struct ProductScoreParam;

namespace faceted { class PropValueTable; }

class BoostLabelSelector
{
public:
    BoostLabelSelector(
        MiningManager& miningManager,
        const faceted::PropValueTable& propValueTable,
        GroupLabelLogger* clickLogger,
        const GroupLabelKnowledge* labelKnowledge);

    bool selectLabel(
        const ProductScoreParam& scoreParam,
        std::size_t limit,
        std::vector<category_id_t>& boostLabels);

private:
    bool getFreqLabel_(
        const std::string& query,
        std::size_t limit,
        std::vector<category_id_t>& boostLabels);

    bool classifyQueryToLabel_(
        const std::string& query,
        std::vector<category_id_t>& boostLabels);

    bool getKnowledgeLabel_(
        const std::string& querySource,
        std::vector<category_id_t>& boostLabels);

private:
    MiningManager& miningManager_;

    const faceted::PropValueTable& propValueTable_;

    GroupLabelLogger* clickLogger_;

    const GroupLabelKnowledge* labelKnowledge_;
};

} // namespace sf1r

#endif // SF1R_BOOST_LABEL_SELECTOR_H
