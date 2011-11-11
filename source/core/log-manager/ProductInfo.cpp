#include "ProductInfo.h"

#include <libcassandra/cassandra.h>

#include <boost/assign/list_of.hpp>

using namespace std;
using namespace boost::assign;
using namespace boost::posix_time;
using namespace libcassandra;
using namespace org::apache::cassandra;

namespace sf1r {

const string ProductInfo::cf_name("ProductInfo");

const string ProductInfo::cf_column_type("Super");

const string ProductInfo::cf_comparator_type;

const string ProductInfo::cf_sub_comparator_type;

const string ProductInfo::cf_comment;

const double ProductInfo::cf_row_cache_size(0);

const double ProductInfo::cf_key_cache_size(200000);

const double ProductInfo::cf_read_repair_chance(1);

const vector<ColumnDef> ProductInfo::cf_column_metadata;

const int32_t ProductInfo::cf_gc_grace_seconds(864000);

const string ProductInfo::cf_default_validation_class;

const int32_t ProductInfo::cf_id(0);

const int32_t ProductInfo::cf_min_compaction_threshold(4);

const int32_t ProductInfo::cf_max_compaction_threshold(22);

const int32_t ProductInfo::cf_row_cache_save_period_in_seconds(0);

const int32_t ProductInfo::cf_key_cache_save_period_in_seconds(0);

const map<string, string> ProductInfo::cf_compression_options
    = map_list_of("sstable_compression", "SnappyCompressor")("chunk_length_kb", "64");

const string ProductInfo::SuperColumnName[] = {"StringProperties", "PriceHistory"};

ProductInfo::ProductInfo(const string& docId)
    : ColumnFamilyBase()
    , docId_(docId)
    , docIdPresent_(!docId.empty())
    , collectionPresent_(false)
    , sourcePresent_(false)
    , titlePresent_(false)
    , priceHistoryPresent_(false)
{}

ProductInfo::~ProductInfo()
{}

bool ProductInfo::updateRow() const
{
    if (!CassandraConnection::instance().isEnabled() || !docIdPresent_) return false;
    try
    {
        boost::shared_ptr<Cassandra> client(CassandraConnection::instance().getCassandraClient());
        string row_key = collectionPresent_ ? collection_ + "_" + docId_ :docId_;
        if (sourcePresent_)
        {
            client->insertColumn(
                    source_,
                    row_key,
                    cf_name,
                    SuperColumnName[0],
                    "Source");
        }
        if (titlePresent_)
        {
            client->insertColumn(
                    title_,
                    row_key,
                    cf_name,
                    SuperColumnName[0],
                    "Title");
        }
        if (priceHistoryPresent_)
        {
            for (PriceHistoryType::const_iterator it = priceHistory_.begin();
                    it != priceHistory_.end(); ++it)
            {
                client->insertColumn(
                        lexical_cast<string>(it->second),
                        row_key,
                        cf_name,
                        SuperColumnName[1],
                        to_iso_string(it->first));
            }
        }
    }
    catch (const InvalidRequestException &ire)
    {
        cout << ire.why << endl;
        return false;
    }
    return true;
}

bool ProductInfo::deleteRow()
{
    if (!CassandraConnection::instance().isEnabled() || !docIdPresent_) return false;
    try
    {
        ColumnPath col_path;
        col_path.__set_column_family(cf_name);
        CassandraConnection::instance().getCassandraClient()->remove(
                collectionPresent_ ? collection_ + "_" + docId_ :docId_,
                col_path);
        reset();
    }
    catch (const InvalidRequestException &ire)
    {
        cout << ire.why << endl;
        return false;
    }
    return true;
}

bool ProductInfo::getRow()
{
    if (!CassandraConnection::instance().isEnabled() || !docIdPresent_) return false;
    try
    {
        boost::shared_ptr<Cassandra> client(CassandraConnection::instance().getCassandraClient());
        string row_key = collectionPresent_ ? collection_ + "_" + docId_ :docId_;

        ColumnParent col_parent;
        col_parent.__set_column_family(cf_name);

        SlicePredicate pred;

        vector<SuperColumn> super_column_list;
        client->getSuperSlice(
                super_column_list,
                row_key,
                col_parent,
                pred);

        for (vector<SuperColumn>::const_iterator sit = super_column_list.begin();
                sit != super_column_list.end(); ++sit)
        {
            if (sit->name == SuperColumnName[0])
            {
                for (vector<Column>::const_iterator cit = sit->columns.begin();
                        cit != sit->columns.end(); ++cit)
                {
                    if (cit->name == "Source")
                        setSource(cit->value);
                    else if (cit->name == "Title")
                        setTitle(cit->value);
                }
            }
            else if (sit->name == SuperColumnName[1] && !sit->columns.empty())
            {
                if (!priceHistoryPresent_)
                {
                    priceHistory_.clear();
                    priceHistoryPresent_ = true;
                }
                for (vector<Column>::const_iterator cit = sit->columns.begin();
                        cit != sit->columns.end(); ++cit)
                {
                    priceHistory_[from_iso_string(cit->name)] = lexical_cast<ProductPriceType>(cit->value);
                }
            }
        }
    }
    catch (const InvalidRequestException &ire)
    {
        cout << ire.why << endl;
        return false;
    }
    return true;
}

void ProductInfo::insertHistory(boost::posix_time::ptime timeStamp, ProductPriceType price)
{
    if (!priceHistoryPresent_)
    {
        priceHistory_.clear();
        priceHistoryPresent_ = true;
    }
    priceHistory_[timeStamp] = price;
}

void ProductInfo::clearHistory()
{
    priceHistory_.clear();
    priceHistoryPresent_ = false;
}

bool ProductInfo::getRangeHistory(PriceHistoryType& history, ptime from, ptime to) const
{
    if (!CassandraConnection::instance().isEnabled() || !docIdPresent_) return false;
    try
    {
        string row_key = collectionPresent_ ? collection_ + "_" + docId_ :docId_;

        ColumnParent col_parent;
        col_parent.__set_column_family(cf_name);
        col_parent.__set_super_column(SuperColumnName[1]);

        SlicePredicate pred;
        if (from != not_a_date_time)
            pred.slice_range.__set_start(to_iso_string(from));
        if (to != not_a_date_time)
            pred.slice_range.__set_finish(to_iso_string(to));

        vector<Column> column_list;
        CassandraConnection::instance().getCassandraClient()->getSlice(
                column_list,
                row_key,
                col_parent,
                pred);

        if (!column_list.empty())
        {
            for (vector<Column>::const_iterator it = column_list.begin();
                    it != column_list.end(); ++it)
            {
                history[from_iso_string(it->name)] = lexical_cast<ProductPriceType>(it->value);
            }
        }
    }
    catch (const InvalidRequestException &ire)
    {
        cout << ire.why << endl;
        return false;
    }
    return true;
}

void ProductInfo::reset(const string& newDocId)
{
    if (!newDocId.empty())
        docId_.assign(newDocId);
    collectionPresent_ = sourcePresent_ = titlePresent_ = priceHistoryPresent_ = false;
}

}
