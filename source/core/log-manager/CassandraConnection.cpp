#include "CassandraConnection.h"

#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <libcassandra/cassandra.h>
#include <libcassandra/connection_manager.h>

using namespace std;
using namespace libcassandra;

namespace sf1r {

CassandraConnection::CassandraConnection()
{
}

CassandraConnection::~CassandraConnection()
{
}

const string& CassandraConnection::getKeyspaceName() const
{
    return keyspace_name_;
}

void CassandraConnection::setKeyspaceName(const string& keyspace_name)
{
    keyspace_name_ = keyspace_name;
}

bool CassandraConnection::init(const std::string& str)
{
    std::string cassandra_prefix = "cassandra://";
    if (boost::algorithm::starts_with(str, cassandra_prefix))
    {
        std::string path = str.substr(cassandra_prefix.length());
        std::string host;
        std::string portStr;
        std::string ks_name;
        uint16_t port = 9160;
        std::string username;
        std::string password;
        bool hasAuth = false;
        static const size_t pool_size = 16;

        // Parse authentication information
        std::size_t pos = path.find('@');
        if (pos == 0)
            goto UNRECOGNIZED;
        if (pos != std::string::npos)
        {
            hasAuth = true;
            std::string auth = path.substr(0, pos);
            path = path.substr(pos + 1);
            pos = auth.find(':');
            if (pos == 0)
                goto UNRECOGNIZED;
            username = auth.substr(0, pos);
            if (pos != std::string::npos)
                password = auth.substr(pos + 1);
        }

        // Parse keyspace name
        pos = path.find('/');
        if (pos == 0)
            goto UNRECOGNIZED;
        if (pos == std::string::npos)
            ks_name= "SF1R";
        else
        {
            ks_name= path.substr(pos + 1);
            path = path.substr(0, pos);
        }
        keyspace_name_ = ks_name;

        // Parse host and port
        pos = path.find(':');
        if (pos == 0)
            goto UNRECOGNIZED;
        host = path.substr(0, pos);
        if (pos == std::string::npos)
            port = 9160;
        else
        {
            path = path.substr(pos + 1);
            portStr = path;
            try
            {
                port = boost::lexical_cast<uint16_t>(portStr);
            }
            catch (std::exception& ex)
            {
                goto UNRECOGNIZED;
            }
        }

        try
        {
            CassandraConnectionManager::instance()->init(host, port, pool_size);
            cassandra_client_.reset(new Cassandra());
            if (hasAuth)
                cassandra_client_->login(username, password);

            KeyspaceDefinition ks_def;
            ks_def.setName(ks_name);
            ks_def.setStrategyClass("NetworkTopologyStrategy");
            // TODO: detail configuration for keyspace
            if (!cassandra_client_->findKeyspace(ks_name))
                cassandra_client_->createKeyspace(ks_def);
            else
                cassandra_client_->updateKeyspace(ks_def);
            cassandra_client_->setKeyspace(ks_name);
        }
        catch (org::apache::cassandra::InvalidRequestException &ire)
        {
            std::cerr << "[CassandraConnection::init] " << ire.why << std::endl;
            return false;
        }
        std::cerr << "[CassandraConnection::init] " << str << std::endl;
        return true;
    }
    else
    {
        UNRECOGNIZED:
        std::cerr << "[CassandraConnection::init] " << str << " unrecognized" << std::endl;
        return false;
    }
}

}
