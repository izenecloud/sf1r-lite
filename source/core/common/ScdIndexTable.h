/*
 * File:   ScdIndexTable.h
 * Author: Paolo D'Apice
 *
 * Created on September 24, 2012, 10:18 AM
 */

#ifndef SCDINDEX_TABLE_H
#define SCDINDEX_TABLE_H

#include "ScdIndexDocument.h"
#include "ScdIndexTableSerializer.h"
#include <3rdparty/am/leveldb/db.h>
#include <3rdparty/am/leveldb/write_batch.h>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>
#include <glog/logging.h>

namespace scd {

namespace ldb = ::leveldb;

/**
 * @brief leveldb adapter to be used for storing ScdIndex.
 */
template <typename Property, typename Docid>
class ScdIndexTable {
    boost::scoped_ptr<ldb::DB> docidDb;
    boost::scoped_ptr<ldb::DB> propertyDb;

public:
    /// Create a new table.
    ScdIndexTable(const std::string& docidDatabase, const std::string& propertyDatabase) {
        docidDb.reset(openDatabase(docidDatabase));
        propertyDb.reset(openDatabase(propertyDatabase));
    }

    /// Destructor.
    ~ScdIndexTable() {}

    /// Create a new record.
    void insert(const Document<Docid, Property>& document) {
        ldb::Status status = insertDocid(document.docid, &document.offset);
        CHECK(status.ok()) << "Cannot insert docid '" << document.docid << "': "
                           << status.ToString();
        status = insertProperty(document.property, &document.offset);
        CHECK(status.ok()) << "Cannot insert property '" << document.property << "': "
                           << status.ToString();
    }

    /// Retrieve file offset of a document.
    bool getByDocid(const typename Docid::type& key, offset_type* offset) const {
        std::string value;
        ldb::Status status = docidDb->Get(ldb::ReadOptions(), key, &value);
        if (status.IsNotFound()) return false;
        CHECK(status.ok()) << "Cannot retrieve value for docid '" << key << "': "
                           << status.ToString();
        *offset = boost::lexical_cast<offset_type>(value);
        return true;
    }

    /// Retrieve file offset of documents containing the given property.
    bool getByProperty(const typename Property::type& key, std::vector<offset_type>& offsets) const {
        std::string value;
        ldb::Status status = propertyDb->Get(ldb::ReadOptions(), key, &value);
        if (status.IsNotFound()) return false;
        CHECK(status.ok()) << "Cannot retrieve values for property '" << key << "': "
                           << status.ToString();
        load(value, offsets);
        return true;
    }

    // Test only
    void dump(std::ostream& os = std::cout) const {
        os << "* by docid:" << std::endl;
        boost::scoped_ptr<ldb::Iterator> it(docidDb->NewIterator(ldb::ReadOptions()));
        for (it->SeekToFirst(); it->Valid(); it->Next()) {
            os << it->key().ToString() << " -> "  << it->value().ToString() << std::endl;
        }
        CHECK(it->status().ok()) << "Error occured during the scan";

        os << "* by property:" << std::endl;
        it.reset(propertyDb->NewIterator(ldb::ReadOptions()));
        for (it->SeekToFirst(); it->Valid(); it->Next()) {
            os << it->key().ToString() << " -> [";
            std::vector<offset_type> offsets;
            load(it->value().ToString(), offsets);
            for (std::vector<offset_type>::const_iterator it = offsets.begin(); it != offsets.end(); ++it)
                os << " " <<*it << " ";
            os << "]" << std::endl;
        }
        CHECK(it->status().ok()) << "Error occured during the scan";
    }

private:
    /// Open a leveldb database.
    ldb::DB* openDatabase(const std::string& filename) const {
        ldb::DB* ptr;
        ldb::Options options;
        options.create_if_missing = true;
        options.error_if_exists = true;
        ldb::Status status = ldb::DB::Open(options, filename, &ptr);
        CHECK(status.ok()) << "Cannot create database to: " << filename;
        return ptr;
    }

    /// Insert a new pair (docid, offset).
    inline ldb::Status
    insertDocid(const typename Docid::type& docid, const offset_type* offset) {
        return docidDb->Put(ldb::WriteOptions(), docid, boost::lexical_cast<std::string>(*offset));
    }

    /// Insert a new pair (property, list<offset>).
    inline ldb::Status
    insertProperty(const typename Property::type& property, const offset_type* offset) {
        std::vector<offset_type> offsets;
        getByProperty(property, offsets);
        offsets.push_back(*offset);

        std::string value;
        save(offsets, value);
        return propertyDb->Put(ldb::WriteOptions(), property, value);
    }

private:
    save_bin save; //< vector serializer
    load_bin load; //< vector deserializer
};

} /* namespace scd */

#endif /* SCDINDEX_TABLE_H */
