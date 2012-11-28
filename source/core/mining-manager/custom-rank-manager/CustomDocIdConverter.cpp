#include "CustomDocIdConverter.h"
#include <common/Utilities.h>
#include <util/ClockTimer.h>

namespace sf1r
{

bool CustomDocIdConverter::convert(
    const CustomRankDocStr& customDocStr,
    CustomRankDocId& customDocId
)
{
    izenelib::util::ClockTimer timer;

    bool topResult = convertDocIdList_(customDocStr.topIds, customDocId.topIds);
    bool excludeResult = convertDocIdList_(customDocStr.excludeIds, customDocId.excludeIds);

    LOG(INFO) << "top num: " << customDocId.topIds.size()
              << ", exclude num: " << customDocId.excludeIds.size()
              << ", id conversion costs: " << timer.elapsed() << " seconds";

    return topResult && excludeResult;
}

bool CustomDocIdConverter::convertDocIdList_(
    const std::vector<std::string>& docStrList,
    std::vector<docid_t>& docIdList
)
{
    docid_t docId = 0;
    bool result = true;

    for (std::vector<std::string>::const_iterator it = docStrList.begin();
        it != docStrList.end(); ++it)
    {
        if (convertDocId(*it, docId))
        {
            docIdList.push_back(docId);
        }
        else
        {
            result = false;
        }
    }

    return result;
}

bool CustomDocIdConverter::convertDocId(
    const std::string& docStr,
    docid_t& docId
)
{
    uint128_t convertId = Utilities::md5ToUint128(docStr);

    if (! idManager_.getDocIdByDocName(convertId, docId, false))
    {
        LOG(WARNING) << "in convertDocId(), DOCID " << docStr
                     << " does not exist";
        return false;
    }

    return true;
}

} // namespace sf1r
