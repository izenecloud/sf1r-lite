#include "CassandraConnection.h"

#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <libcassandra/cassandra.h>
#include <libcassandra/connection_manager.h>

using namespace std;
using namespace org::apache::cassandra;
using namespace libcassandra;

namespace sf1r {

const int CassandraConnection::PoolSize = 16;

CassandraConnection::CassandraConnection()
{
}

CassandraConnection::~CassandraConnection()
{
}

bool CassandraConnection::init(const std::string& str)
{
    std::string cassandra_prefix = "cassandra://";
    if (boost::algorithm::starts_with(str, cassandra_prefix))
    {
        std::string path = str.substr(cassandra_prefix.length());
        std::string host;
        std::string portStr;
        uint16_t port = 9160;
//      std::string username;
//      std::string password;

        std::size_t pos = path.find(":");
//      if (pos == 0 || pos == std::string::npos) return false;
//      username = path.substr(0, pos);
//      path = path.substr(pos + 1);
//      pos = path.find("@");
//      if (pos == std::string::npos) return false;
//      password = path.substr(0, pos);
//      path = path.substr(pos + 1);
//      pos = path.find(":");
        if (pos == 0 || pos == std::string::npos) return false;
        host = path.substr(0, pos);
        path = path.substr(pos + 1);
//      pos = path.find("/");
//      if (pos == 0) return false;
        portStr = path;
        try
        {
            port = boost::lexical_cast<uint16_t>(portStr);
        }
        catch (std::exception& ex)
        {
            return false;
        }

        try
        {
            CassandraConnectionManager::instance()->init(host, port, PoolSize);
            cassandraClient_.reset(new Cassandra());
 //         if (!username.empty() || !password.empty())
 //             cassandraClient_->login(username, password);
        }
        catch (InvalidRequestException &ire)
        {
            std::cerr << ire.why << std::endl;
            return false;
        }
        return true;
    }
    else
    {
        std::cerr << "[DbConnection::init] " << str << " unrecognized" << std::endl;
        return false;
    }
}

}
