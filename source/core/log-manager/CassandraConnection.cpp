#include "CassandraConnection.h"

#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <libcassandra/cassandra.h>
#include <libcassandra/connection_manager.h>
#include <libcassandra/util_functions.h>

using namespace std;
using namespace libcassandra;
using namespace org::apache::cassandra;

namespace sf1r {

CassandraConnection::CassandraConnection()
    : isEnabled_(false)
{
}

CassandraConnection::~CassandraConnection()
{
}

bool CassandraConnection::isEnabled()
{
    return isEnabled_;
}

boost::shared_ptr<libcassandra::Cassandra>& CassandraConnection::getCassandraClient()
{
    return cassandra_client_;
}

time_t CassandraConnection::createTimestamp()
{
    return libcassandra::createTimestamp();
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
    if (str == "__disabled__") return false;
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
        if (pos == 0) goto UNRECOGNIZED;
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
        if (pos == 0) goto UNRECOGNIZED;
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
        if (pos == 0) goto UNRECOGNIZED;
        host = path.substr(0, pos);
        if (pos != std::string::npos)
        {
            path = path.substr(pos + 1);
            portStr = path;
            try
            {
                port = boost::lexical_cast<uint16_t>(portStr);
            }
            catch (const boost::bad_lexical_cast &)
            {
                goto UNRECOGNIZED;
            }
        }

        // Try to connect to and create keyspace on server
        try
        {
            CassandraConnectionManager::instance()->init(host, port, pool_size);
            cassandra_client_.reset(new Cassandra());
            if (hasAuth)
                cassandra_client_->login(username, password);
            KeyspaceDefinition ks_def;
            ks_def.setName(ks_name);
            ks_def.setStrategyClass("NetworkTopologyStrategy");
            // TODO: more tricks for keyspace
            cassandra_client_->createKeyspace(ks_def);
            cassandra_client_->setKeyspace(ks_name);
        }
        catch (const ::apache::thrift::TException &ex)
        {
            std::cerr << "[CassandraConnection::init] " << ex.what() << std::endl;
            return false;
        }
        std::cerr << "[CassandraConnection::init] " << str << std::endl;
        isEnabled_ = true;
        return true;
    }
    else
    {
        UNRECOGNIZED:
        std::cerr << "[CassandraConnection::init] " << str << " unrecognized" << std::endl;
        return false;
    }
}

bool CassandraConnection::createColumnFamily(
        const string& in_name,
        const string& in_column_type,
        const string& in_comparator_type,
        const string& in_sub_comparator_type,
        const string& in_comment,
        const double in_row_cache_size,
        const double in_key_cache_size,
        const double in_read_repair_chance,
        const vector<ColumnDef>& in_column_metadata,
        const int32_t in_gc_grace_seconds,
        const string& in_default_validation_class,
        const int32_t in_id,
        const int32_t in_min_compaction_threshold,
        const int32_t in_max_compaction_threshold,
        const int32_t in_row_cache_save_period_in_seconds,
        const int32_t in_key_cache_save_period_in_seconds,
        const map<string, string>& in_compression_options)
{
    if (!isEnabled_) return false;
    ColumnFamilyDefinition definition(
                keyspace_name_,
                in_name,
                in_column_type,
                in_comparator_type,
                in_sub_comparator_type,
                in_comment,
                in_row_cache_size,
                in_key_cache_size,
                in_read_repair_chance,
                in_column_metadata,
                in_gc_grace_seconds,
                in_default_validation_class,
                in_id,
                in_min_compaction_threshold,
                in_max_compaction_threshold,
                in_row_cache_save_period_in_seconds,
                in_key_cache_save_period_in_seconds,
                in_compression_options);
    try
    {
        cassandra_client_->createColumnFamily(definition);
    }
    catch (const InvalidRequestException& ire)
    {
        cerr << "[CassandraConnection::init] error: " << ire.why << endl;
        return false;
    }
    return true;
}

bool CassandraConnection::truncateColumnFamily(const string& in_name)
{
    if (!isEnabled_) return false;
    try
    {
        cassandra_client_->truncateColumnFamily(in_name);
    }
    catch (const InvalidRequestException& ire)
    {
        cerr << ire.why << endl;
        return false;
    }
    return true;
}

bool CassandraConnection::dropColumnFamily(const string& in_name)
{
    if (!isEnabled_) return false;
    try
    {
        cassandra_client_->dropColumnFamily(in_name);
    }
    catch (const InvalidRequestException& ire)
    {
        cerr << ire.why << endl;
        return false;
    }
    return true;
}

}
