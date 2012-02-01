#include "CassandraConnection.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/locks.hpp>

#include <libcassandra/cassandra.h>
#include <libcassandra/connection_manager.h>
#include <libcassandra/util_functions.h>

#include <memory> // auto_ptr

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
    clear_();
}

void CassandraConnection::clear_()
{
    isEnabled_ = false;
    user_name_.clear();
    password_.clear();

    for (KeyspaceClientMap::iterator it = client_map_.begin();
        it != client_map_.end(); ++it)
    {
        delete it->second;
    }
    client_map_.clear();
}

bool CassandraConnection::isEnabled()
{
    return isEnabled_;
}

bool CassandraConnection::init(const string& str)
{
    clear_();

    if (str == "__disabled__") return false;
    string cassandra_prefix = "cassandra://";
    if (boost::algorithm::starts_with(str, cassandra_prefix))
    {
        string path = str.substr(cassandra_prefix.length());
        string host;
        string portStr;
        uint16_t port = 9160;
        string username;
        string password;
        static const size_t pool_size = 16;

        // Parse authentication information
        size_t pos = path.find('@');
        if (pos == 0) goto UNRECOGNIZED;
        if (pos != string::npos)
        {
            string auth = path.substr(0, pos);
            path = path.substr(pos + 1);
            pos = auth.find(':');
            if (pos == 0)
                goto UNRECOGNIZED;
            user_name_ = auth.substr(0, pos);
            if (pos != string::npos)
                password_ = auth.substr(pos + 1);
        }

        // Parse host and port
        pos = path.find(':');
        if (pos == 0) goto UNRECOGNIZED;
        host = path.substr(0, pos);
        if (pos != string::npos)
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

        // Try to connect to server
        try
        {
            CassandraConnectionManager::instance()->init(host, port, pool_size);
        }
        CATCH_CASSANDRA_EXCEPTION("[CassandraConnection::init] " + str);

        cout << "[CassandraConnection::init] " << "successfully connected to " << str << endl;
        isEnabled_ = true;
    }
    else
    {
        UNRECOGNIZED:
        cerr << "[CassandraConnection::init] " << str << " unrecognized" << endl;
        return false;
    }
    return true;
}

libcassandra::Cassandra* CassandraConnection::getCassandraClient(const std::string& keyspace_name)
{
    libcassandra::Cassandra* client = getClient_(keyspace_name);

    if (! client)
    {
        client = createClient_(keyspace_name);
    }

    return client;
}

libcassandra::Cassandra* CassandraConnection::getClient_(const std::string& keyspace_name)
{
    boost::shared_lock<boost::shared_mutex> lock(keyspace_mutex_);

    KeyspaceClientMap::const_iterator it = client_map_.find(keyspace_name);
    if (it != client_map_.end())
        return it->second;

    return NULL;
}

libcassandra::Cassandra* CassandraConnection::createClient_(const std::string& keyspace_name)
{
    if (! isEnabled_)
        return NULL;

    std::auto_ptr<Cassandra> clientPtr(new Cassandra);
    if (! initClient_(clientPtr.get(), keyspace_name))
        return NULL;

    boost::unique_lock<boost::shared_mutex> lock(keyspace_mutex_);
    libcassandra::Cassandra*& client = client_map_[keyspace_name];
    if (! client)
    {
        client = clientPtr.release();
    }

    return client;
}

bool CassandraConnection::initClient_(libcassandra::Cassandra* client, const std::string& keyspace_name)
{
    try
    {
        if (! user_name_.empty())
            client->login(user_name_, password_);

        KsDef ks_def;
        ks_def.__set_name(keyspace_name);
        ks_def.__set_strategy_class("SimpleStrategy");
        map<string, string> strategy_options;
        strategy_options["replication_factor"] = "1";
        ks_def.__set_strategy_options(strategy_options);

        client->createKeyspace(ks_def);
        client->setKeyspace(keyspace_name);
    }
    CATCH_CASSANDRA_EXCEPTION("[CassandraConnection::initClient_] error: ");

    return true;
}

bool CassandraConnection::dropKeyspace(const std::string& keyspace_name)
{
    try
    {
        Cassandra client;
        client.dropKeyspace(keyspace_name);
    }
    CATCH_CASSANDRA_EXCEPTION("[CassandraConnection::dropKeyspace] error: ");

    boost::unique_lock<boost::shared_mutex> lock(keyspace_mutex_);

    KeyspaceClientMap::iterator it = client_map_.find(keyspace_name);
    if (it != client_map_.end())
    {
        delete it->second;
        client_map_.erase(it);
    }

    return true;
}

bool CassandraConnection::createColumnFamily(
        const std::string& in_keyspace_name,
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
        const int8_t in_replicate_on_write,
        const double in_merge_shards_chance,
        const string& in_key_validation_class,
        const string& in_row_cache_provider,
        const string& in_key_alias,
        const string& in_compaction_strategy,
        const map<string, string>& in_compaction_strategy_options,
        const int32_t in_row_cache_keys_to_save,
        const map<string, string>& in_compression_options)
{
    if (! isEnabled_)
        return false;

    Cassandra* client = getCassandraClient(in_keyspace_name);
    if (! client)
        return false;

    CfDef definition;
    createCfDefObject(
            definition,
            in_keyspace_name,
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
            in_replicate_on_write,
            in_merge_shards_chance,
            in_key_validation_class,
            in_row_cache_provider,
            in_key_alias,
            in_compaction_strategy,
            in_compaction_strategy_options,
            in_row_cache_keys_to_save,
            in_compression_options);
    try
    {
        client->createColumnFamily(definition);
    }
    CATCH_CASSANDRA_EXCEPTION("[CassandraConnection::init] error: ");

    return true;
}

}
