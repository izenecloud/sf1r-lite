#ifndef SF1R_PARSERS_FILTERING_PARSER_HELPER_H
#define SF1R_PARSERS_FILTERING_PARSER_HELPER_H

#include <common/BundleSchemaHelpers.h>

// #include "FilteringParser.h"
#include <util/driver/Parser.h>
#include <util/driver/Value.h>
#include <common/ValueConverter.h>
#include <bundles/index/IndexBundleConfiguration.h>
#include <bundles/mining/MiningBundleConfiguration.h>
#include <query-manager/QueryTypeDef.h>
#include <common/parsers/ConditionArrayParser.h>
#include <string>
#include <stack>

namespace sf1r {


QueryFiltering::FilteringOperation toFilteringOperation(
    const std::string& op
);


bool do_paser(const ConditionParser& condition,
            const IndexBundleSchema& indexSchema,
            QueryFiltering::FilteringType& filteringRule);

}

#endif // SF1R_PARSERS_FILTERING_PARSER_HELPER_H
