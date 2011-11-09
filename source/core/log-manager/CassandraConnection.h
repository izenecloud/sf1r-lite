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
        return cassandra_client_;
    }

public:
    enum COLUMN_FAMILY_ID {
        PRODUCT_INFO = 1000,
        CUSTOMER_INFO = 2000,
        VENDOR_INFO = 3000,
        SYSTEM_INFO = 4000,
    };

private:
    static const int pool_size;

    boost::shared_ptr<libcassandra::Cassandra> cassandra_client_;
};

}

#endif
