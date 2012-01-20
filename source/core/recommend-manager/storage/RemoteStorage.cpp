#include "RemoteStorage.h"

#include <glog/logging.h>

namespace sf1r
{

RemoteStorage::RemoteStorage(
    const std::string& keyspace,
    const std::string& collection,
    const std::string& columnFamilySuffix,
    const org::apache::cassandra::CfDef& cfDef
)
    : keyspace_(keyspace)
    , collection_(collection)
    , columnFamily_(collection + "_" + columnFamilySuffix)
    , cassandra_(keyspace_, columnFamily_)
{
    org::apache::cassandra::CfDef def(cfDef);

    def.__set_keyspace(keyspace_);
    def.__set_name(columnFamily_);

    if (! cassandra_.createColumnFamily(def))
    {
        LOG(ERROR) << "failed to create column family " << columnFamily_;
    }
}

} // namespace sf1r
