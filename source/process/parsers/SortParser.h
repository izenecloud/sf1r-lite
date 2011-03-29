#ifndef SF1R_PARSER_SORT_PARSER_H
#define SF1R_PARSER_SORT_PARSER_H
/**
 * @file core/common/parsers/SortParser.h
 * @author Ian Yang
 * @date Created <2010-07-12 12:15:27>
 */

#include <util/driver/Parser.h>
#include <util/driver/Value.h>

#include <common/parsers/OrderArrayParser.h>

#include <bundles/index/IndexBundleConfiguration.h>

#include <string>

namespace sf1r {
/// @addtogroup parsers
/// @{

/**
 * @brief Order array parser with schema validation.
 */
using namespace izenelib::driver; 
class SortParser : public OrderArrayParser
{
public:
    explicit SortParser(const IndexBundleSchema& indexSchema)
    : indexSchema_(indexSchema)
    {}

    bool parse(const Value& orders);

    const std::vector<std::pair<std::string , bool> >&
    sortPropertyList() const
    {
        return sortPriorityList_;
    }

    std::vector<std::pair<std::string , bool> >&
    mutableSortPropertyList()
    {
        return sortPriorityList_;
    }

private:
    const IndexBundleSchema& indexSchema_;

    std::vector<std::pair<std::string , bool> > sortPriorityList_;
};


/// @}

} // namespace sf1r

#endif // SF1R_PARSER_SORT_PARSER_H
