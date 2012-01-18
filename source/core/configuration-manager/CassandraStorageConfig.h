#ifndef SF1R_CASSANDRA_STORAGE_CONFIG_H_
#define SF1R_CASSANDRA_STORAGE_CONFIG_H_

#include <string>

namespace sf1r
{

/**
 * @brief parameters for cassandra storage
 */
struct CassandraStorageConfig
{
    bool enable;

    std::string keyspace;

    CassandraStorageConfig()
        : enable(false)
        , keyspace("SF1R")
    {}
};

} // namespace

#endif // SF1R_CASSANDRA_STORAGE_CONFIG_H_
