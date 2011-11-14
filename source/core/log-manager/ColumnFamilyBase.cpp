#include "ColumnFamilyBase.h"

#include <libcassandra/cassandra.h>

using namespace std;
using namespace libcassandra;
using namespace org::apache::cassandra;

namespace sf1r {

bool ColumnFamilyBase::deleteRow()
{
    if (!CassandraConnection::instance().isEnabled() || getKey().empty()) return false;
    try
    {
        ColumnPath col_path;
        col_path.__set_column_family(getName());
        CassandraConnection::instance().getCassandraClient()->remove(
                getKey(),
                col_path);
        resetKey();
    }
    catch (const InvalidRequestException &ire)
    {
        cout << ire.why << endl;
        return false;
    }
    return true;
}

bool ColumnFamilyBase::getCount(int32_t& count, const string& start, const string& finish) const
{
    if (!CassandraConnection::instance().isEnabled() || getKey().empty()) return false;
    try
    {
        ColumnParent col_parent;
        col_parent.__set_column_family(getName());

        SlicePredicate pred;
        //pred.slice_range.__set_count(numeric_limits<int32_t>::max());
        pred.slice_range.__set_start(start);
        pred.slice_range.__set_finish(finish);

        count = CassandraConnection::instance().getCassandraClient()->getCount(
                getKey(),
                col_parent,
                pred);
    }
    catch (const InvalidRequestException &ire)
    {
        cout << ire.why << endl;
        return false;
    }
    return true;
}

}
