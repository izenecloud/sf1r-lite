#ifndef _CASSANDRA_CONNECTION_H_
#define _CASSANDRA_CONNECTION_H_

#include <string>
#include <map>
#include <list>

#include <util/ThreadModel.h>

#include "LogManagerSingleton.h"

namespace libcassandra {
class Cassandra;
}

namespace sf1r {

class CassandraConnection : public LogManagerSingleton<CassandraConnection>
{
public:
    CassandraConnection();

    ~CassandraConnection();

    bool init(const std::string& str);

    boost::shared_ptr<libcassandra::Cassandra>& getCassandraClient()
    {
        return cassandraClient_;
    }

private:
    static const int PoolSize;

    boost::shared_ptr<libcassandra::Cassandra> cassandraClient_;
};

}

#endif
