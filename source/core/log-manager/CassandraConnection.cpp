#include "CassandraConnection.h"

#include <libcassandra/connection_manager.h>

#include <vector>
#include <iostream>
#include <boost/lexical_cast.hpp>

using namespace sf1r;
using namespace org::apache::cassandra;
using namespace libcassandra;

const int CassandraConnection::PoolSize = 16;

CassandraConnection::CassandraConnection()
{
//    sqlKeywords_[RDbConnection::ATTR_AUTO_INCREMENT] = "AUTO_INCREMENT";
 //   sqlKeywords_[RDbConnection::FUNC_LAST_INSERT_ID] = "last_insert_id()";
}

CassandraConnection::~CassandraConnection()
{
    close();
}

void CassandraConnection::close()
{
}

bool CassandraConnection::init(const std::string& str)
{
    std::string host;
    std::string portStr;
    uint16_t port = 9160;
    std::string username;
    std::string password;

    std::string conn = str;
    std::size_t pos = conn.find(":");
    if (pos == 0 || pos == std::string::npos) return false;
    username = conn.substr(0, pos);
    conn = conn.substr(pos + 1);
    pos = conn.find("@");
    if(pos == std::string::npos) return false;
    password = conn.substr(0, pos);
    conn = conn.substr(pos+1);
    pos = conn.find(":");
    if (pos == 0 || pos == std::string::npos) return false;
    host = conn.substr(0, pos);
    conn = conn.substr(pos + 1);
    pos = conn.find("/");
    if(pos==0) return false;
    portStr = conn;
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
        cassandraClient.reset(new Cassandra());
        cassandraClient->login(username, password);
    }
    catch (InvalidRequestException &ire)
    {
        std::cerr << ire.why << std::endl;
        return false;
    }

    return true;
}
