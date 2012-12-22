#include "BoostLabelSelector.h"
#include "ProductScoreParam.h"
#include "../MiningManager.h"
#include "../group-label-logger/GroupLabelLogger.h"
#include "../group-label-logger/BackendLabel2FrontendLabel.h"
#include "../group-label-logger/GroupLabelKnowledge.h"
#include "../util/split_ustr.h"
#include <util/ustring/UString.h>

using namespace sf1r;

namespace
{
const izenelib::util::UString::EncodingType kEncodingType =
    izenelib::util::UString::UTF_8;
}

BoostLabelSelector::BoostLabelSelector(
    MiningManager& miningManager,
    const faceted::PropValueTable& propValueTable,
    GroupLabelLogger* clickLogger,
    const GroupLabelKnowledge* labelKnowledge)
    : miningManager_(miningManager)
    , propValueTable_(propValueTable)
    , clickLogger_(clickLogger)
    , labelKnowledge_(labelKnowledge)
{
}

bool BoostLabelSelector::selectLabel(
    const ProductScoreParam& scoreParam,
    std::size_t limit,
    std::vector<category_id_t>& boostLabels)
{
    if (getFreqLabel_(scoreParam.query_, limit, boostLabels) ||
        classifyQueryToLabel_(scoreParam.query_, boostLabels) ||
        getKnowledgeLabel_(scoreParam.querySource_, boostLabels))
    {
        if (boostLabels.size() > limit)
        {
            boostLabels.resize(limit);
        }
    }

    return !boostLabels.empty();
}

bool BoostLabelSelector::getFreqLabel_(
    const std::string& query,
    std::size_t limit,
    std::vector<category_id_t>& boostLabels)
{
    if (clickLogger_)
    {
        std::vector<int> topFreqs;
        clickLogger_->getFreqLabel(query, limit, boostLabels, topFreqs);
    }

    return !boostLabels.empty();
}

bool BoostLabelSelector::classifyQueryToLabel_(
    const std::string& query,
    std::vector<category_id_t>& boostLabels)
{
    const izenelib::util::UString ustrQuery(query, kEncodingType);
    izenelib::util::UString backendCategory;
    if (!miningManager_.GetProductCategory(ustrQuery, backendCategory))
        return false;

    izenelib::util::UString frontendCategory;
    if (!BackendLabelToFrontendLabel::Get()->Map(backendCategory,
                                                 frontendCategory))
        return false;

    std::vector<std::vector<izenelib::util::UString> > groupPaths;
    split_group_path(frontendCategory, groupPaths);
    if (groupPaths.empty())
        return false;

    faceted::PropValueTable::pvid_t topLabel =
        propValueTable_.propValueId(groupPaths[0]);
    if (topLabel == 0)
        return false;

    boostLabels.push_back(topLabel);
    return true;
}

bool BoostLabelSelector::getKnowledgeLabel_(
    const std::string& querySource,
    std::vector<category_id_t>& boostLabels)
{
    if (labelKnowledge_ && !querySource.empty())
    {
        labelKnowledge_->getKnowledgeLabel(querySource, boostLabels);
    }

    return !boostLabels.empty();
}
