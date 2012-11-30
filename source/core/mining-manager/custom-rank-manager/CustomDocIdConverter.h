/**
 * @file CustomDocIdConverter.h
 * @brief convert from CustomRankDocStr to CustomRankDocId.
 * @author Jun Jiang <jun.jiang@izenesoft.com>
 * @date Created 2012-08-14
 */

#ifndef SF1R_CUSTOM_DOC_ID_CONVERTER_H
#define SF1R_CUSTOM_DOC_ID_CONVERTER_H

#include "CustomRankValue.h"
#include <ir/id_manager/IDManager.h>

namespace sf1r
{

class CustomDocIdConverter
{
public:
    CustomDocIdConverter(izenelib::ir::idmanager::IDManager& idManager)
        : idManager_(idManager)
    {}

    /**
     * convert from @p customDocStr to @p customDocId.
     * @return true for success, false for failure
     */
    bool convert(
        const CustomRankDocStr& customDocStr,
        CustomRankDocId& customDocId
    );

    bool convertDocId(
        const std::string& docStr,
        docid_t& docId
    );

private:
    bool convertDocIdList_(
        const std::vector<std::string>& docStrList,
        std::vector<docid_t>& docIdList
    );

private:
    izenelib::ir::idmanager::IDManager& idManager_;
};

} // namespace sf1r

#endif // SF1R_CUSTOM_DOC_ID_CONVERTER_H
