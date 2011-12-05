#ifndef SF1R_PROCESS_COMMON_MININGBUNDLE_SCHEMA_HELPER_H
#define SF1R_PROCESS_COMMON_MININGBUNDLE_SCHEMA_HELPER_H
/**
 * @file process/common/MiningBundleSchemaHelpers.h
 * @brief Helper functions for MiningBundleSchema
 */

#include <bundles/mining/MiningBundleConfiguration.h>

#include <vector>
#include <string>

namespace sf1r
{

bool isPropertyForeignKey(
    const MiningSchema& schema,
    const std::string& property
);


} // namespace sf1r

#endif // SF1R_PROCESS_COMMON_MININGBUNDLE_SCHEMA_HELPER_H
