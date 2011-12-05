/**
 * @file core/common/MiningBundleSchemaHelpers.cpp
 */
#include "MiningBundleSchemaHelpers.h"

#include <boost/algorithm/string/compare.hpp>

namespace sf1r
{

bool isPropertyForeignKey(
    const MiningSchema& schema,
    const std::string& property
)
{
    if (schema.summarization_enable &&
            !schema.summarization_schema.parentKey.empty())
    {
        return boost::is_iequal()(property,
                                schema.summarization_schema.parentKey);
    }

    return false;
}

} // NAMESPACE sf1r


