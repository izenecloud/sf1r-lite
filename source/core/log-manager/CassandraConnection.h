#ifndef SF1R_CASSANDRA_CONNECTION_H_
#define SF1R_CASSANDRA_CONNECTION_H_

#include <libcassandra/cassandra.h>

#include <util/ThreadModel.h>
#include "NoSqlConnectionBase.h"

namespace sf1r {

class CassandraConnection : public NoSqlConnectionBase
{
public:

    CassandraConnection();

    ~CassandraConnection();

    bool init(const std::string& str);

    /// close all database connections
    void close();

private:
    static const int PoolSize;

    boost::shared_ptr<libcassandra::Cassandra> cassandraClient;
};

}

#endif
