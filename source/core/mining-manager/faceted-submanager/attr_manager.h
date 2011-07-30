///
/// @file attr_manager.h
/// @brief calculate doc count for each attribute value
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-06-22
///

#ifndef SF1R_ATTR_MANAGER_H_
#define SF1R_ATTR_MANAGER_H_

#include "ontology_rep.h"
#include "attr_table.h"
#include <configuration-manager/AttrConfig.h>
#include <util/ustring/UString.h>

#include <vector>
#include <string>

namespace sf1r
{
class DocumentManager;
}

NS_FACETED_BEGIN

class AttrManager {
public:
    AttrManager(
        DocumentManager* documentManager,
        const std::string& dirPath
    );

    /**
     * @brief Open the property which need group result for attribute value.
     * @param attrConfig the property config
     * @return true for success, false for failure
     */
    bool open(const AttrConfig& attrConfig);

    /**
     * @brief Build group index data for the whole collection.
     * @return true for success, false for failure
     */
    bool processCollection();

    const AttrTable& getAttrTable() const
    {
        return attrTable_;
    }

    typedef std::pair<
                izenelib::util::UString, /// attribute name
                std::vector<izenelib::util::UString> /// attribute values
            > AttrPair;

    /**
     * Split @p src into pairs of attribute name and values.
     * @param src the source string, such as "品牌:欧艾尼,年份:2011,尺码:S|M|L|XL"
     * @param attrPairs stores the pairs of attribute name and values, for example,
     *                  [0]: attribute name "品牌", values: "欧艾尼",
     *                  [1]: attribute name "年份", values: "2011",
     *                  [2]: attribute name "尺码", values: ["S", "M", "L", "XL"]
     */
    static void splitAttrPair(
        const izenelib::util::UString& src,
        std::vector<AttrPair>& attrPairs
    );

private:
    sf1r::DocumentManager* documentManager_;
    std::string dirPath_;

    AttrTable attrTable_;
};

NS_FACETED_END

#endif // SF1R_ATTR_MANAGER_H_
