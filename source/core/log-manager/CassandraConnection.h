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

    const std::string& getKeyspaceName() const;

    void setKeyspaceName(const std::string& keyspace_name);

    bool init(const std::string& str);

    boost::shared_ptr<libcassandra::Cassandra>& getCassandraClient()
    {
        return cassandra_client_;
    }

public:
    enum COLUMN_FAMILY_ID {
        PRODUCT_INFO = 2000,
        CUSTOMER_INFO,
        VENDOR_INFO,
        SYSTEM_INFO,
    };

private:
    std::string keyspace_name_;

    boost::shared_ptr<libcassandra::Cassandra> cassandra_client_;
};

}

#endif
