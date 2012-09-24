/*
 * File:   ScdIndexTable.h
 * Author: Paolo D'Apice
 *
 * Created on September 24, 2012, 10:18 AM
 */

#ifndef SCDINDEX_TABLE_H
#define SCDINDEX_TABLE_H

#include "ScdIndexDocument.h"
#include "ScdIndexLeveldbTags.h"
#include <3rdparty/am/leveldb/db.h>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>
#include <glog/logging.h>

namespace scd {
namespace leveldb {

namespace ldb = ::leveldb;

/**
 * @brief leveldb adapter to be used for storing ScdIndex.
 */
template <typename Property, typename Docid>
class ScdIndexTable {
    boost::scoped_ptr<ldb::DB> docidDb;
    //boost::scoped_ptr<ldb::DB> propertyDb;

public:
    /// Create a new table.
    ScdIndexTable(const std::string& docidDatabase /*, const std::string& propertyDatabase */) {
        docidDb.reset(openDatabase(docidDatabase));
        //propertyDb.reset(openDatabase(propertyDatabase));
        DLOG(INFO) << "connected to leveldb databases";
    }

    /// Destructor.
    ~ScdIndexTable() {}

//private: // TODO fix access for tests

    /// Create a new record.
    void insert(const Document<Docid, Property>& document) {
        ldb::Status status = docidDb->Put(ldb::WriteOptions(), document.docid, boost::lexical_cast<std::string>(document.offset));
        CHECK(status.ok()) << "Cannot insert value for key: " << document.docid;
    }

    /// Retrieve an existing record.
    offset_type get(const typename Docid::type key) const {
        std::string value;
        ldb::Status status = docidDb->Get(ldb::ReadOptions(), key, &value);
        CHECK(status.ok()) << "Cannot retrieve value for key: " << key;

        return boost::lexical_cast<offset_type>(value);
    }

#if 0
    /// Update an existing record.
    template <typename Key, typename Value>
    void update(const Key& key, const Value& value) {
        CHECK(false) << "not yet implemented";
    }
#endif

    // Test only
    void dump(std::ostream& os = std::cout) const {
        boost::scoped_ptr<ldb::Iterator> it(docidDb->NewIterator(ldb::ReadOptions()));
        for (it->SeekToFirst(); it->Valid(); it->Next()) {
            os << it->key().ToString() << ": "  << it->value().ToString() << std::endl;
        }
        CHECK(it->status().ok()) << "Error occured during the scan";
    }

private:
    ldb::DB* openDatabase(const std::string& filename) const {
        ldb::DB* ptr;
        ldb::Options options;
        options.create_if_missing = true;
        options.error_if_exists = true;
        ldb::Status status = ldb::DB::Open(options, filename, &ptr);
        CHECK(status.ok()) << "Cannot create database to: " << filename;
        return ptr;
    }
};

}} /* namespace scd::leveldb */

#endif /* SCDINDEX_TABLE_H */
