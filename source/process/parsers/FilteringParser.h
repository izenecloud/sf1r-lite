#ifndef SF1R_PARSERS_FILTERING_PARSER_H
#define SF1R_PARSERS_FILTERING_PARSER_H
/**
 * @file core/common/parsers/FilteringParser.h
 * @author Ian Yang
 * @date Created <2010-06-11 17:22:42>
 */
#include <util/driver/Parser.h>
#include <util/driver/Value.h>

#include <bundles/index/IndexBundleConfiguration.h>
#include <bundles/mining/MiningBundleConfiguration.h>
#include <query-manager/QueryTypeDef.h>

#include <string>

namespace sf1r {

using namespace izenelib::driver;
class FilteringParser : public ::izenelib::driver::Parser
{
public:
    explicit FilteringParser(
        const IndexBundleSchema& indexSchema,
        const MiningSchema& miningSchema)
    : indexSchema_(indexSchema)
    , miningSchema_(miningSchema)
    {}

    bool parse(const Value& conditions);
    
    std::vector<QueryFiltering::FilteringType>&
    mutableFilteringRules()
    {
        return filteringRules_;
    }

    const std::vector<QueryFiltering::FilteringType>&
    filteringRules() const
    {
        return filteringRules_;
    }

    static QueryFiltering::FilteringOperation toFilteringOperation(
        const std::string& op
    );
    
private:
    const IndexBundleSchema& indexSchema_;
    const MiningSchema& miningSchema_;

    std::vector<QueryFiltering::FilteringType> filteringRules_;
};

} // namespace sf1r

#endif // SF1R_PARSERS_FILTERING_PARSER_H
