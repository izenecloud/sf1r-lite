#include "CassandraConnection.h"

#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <libcassandra/cassandra.h>
#include <libcassandra/connection_manager.h>

using namespace std;
using namespace libcassandra;
using namespace org::apache::cassandra;

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
            cassandra_client_->createKeyspace(ks_def);
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

bool CassandraConnection::createColumnFamily(
        const string& in_name,
        const string& in_column_type,
        const string& in_comparator_type,
        const string& in_sub_comparator_type,
        const string& in_comment,
        const double in_row_cache_size,
        const double in_key_cache_size,
        const double in_read_repair_chance,
        const vector<ColumnDefFake>& in_column_metadata,
        const int32_t in_gc_grace_seconds,
        const string& in_default_validation_class,
        const int32_t in_id,
        const int32_t in_min_compaction_threshold,
        const int32_t in_max_compaction_threshold,
        const int32_t in_row_cache_save_period_in_seconds,
        const int32_t in_key_cache_save_period_in_seconds,
        const map<string, string>& in_compression_options)
{
    vector<ColumnDef> column_metadata;
    for (vector<ColumnDefFake>::const_iterator it = in_column_metadata.begin();
            it != in_column_metadata.end(); ++it)
    {
        ColumnDef col_def;
        if (!it->name_.empty())
            col_def.__set_name(it->name_);
        if (!it->validation_class_.empty())
            col_def.__set_validation_class(it->validation_class_);
        if (it->index_type_ != ColumnDefFake::UNSET)
            col_def.__set_index_type((IndexType::type) it->index_type_);
        if (!it->index_name_.empty())
            col_def.__set_index_name(it->index_name_);
        if (!it->index_options_.empty())
            col_def.__set_index_options(it->index_options_);
        column_metadata.push_back(col_def);
    }
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
                column_metadata,
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
    catch (InvalidRequestException& ire)
    {
        cerr << "[CassandraConnection::init] error: " << ire.why << endl;
        return false;
    }
    return true;
}

bool CassandraConnection::truncateColumnFamily(const string& in_name)
{
    try
    {
        cassandra_client_->truncateColumnFamily(in_name);
    }
    catch (InvalidRequestException& ire)
    {
        cerr << ire.why << endl;
        return false;
    }
    return true;
}

bool CassandraConnection::dropColumnFamily(const string& in_name)
{
    try
    {
        cassandra_client_->dropColumnFamily(in_name);
    }
    catch (InvalidRequestException& ire)
    {
        cerr << ire.why << endl;
        return false;
    }
    return true;
}

}
