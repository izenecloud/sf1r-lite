/*
 * File:   ScdIndexLeveldb.h
 * Author: Paolo D'Apice
 *
 * Created on September 24, 2012, 10:18 AM
 */

#ifndef SCDINDEX_LEVELDB_H
#define SCDINDEX_LEVELDB_H

#include "ScdIndexDocument.h"
#include <3rdparty/am/leveldb/db.h>
#include <3rdparty/am/leveldb/write_batch.h>
#include <am/leveldb/Table.h>
#include <am/range/AmIterator.h>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>
#include <glog/logging.h>

namespace scd {

/**
 * @brief leveldb adapter to be used for storing ScdIndex.
 */
template <typename Docid, typename Property>
class ScdIndexLeveldb {
    typedef std::vector<offset_type> offset_list;
    typedef izenelib::am::leveldb::Table<typename Docid::type, offset_type> DocidTable;
    typedef izenelib::am::leveldb::Table<typename Property::type, offset_list> PropertyTable;

public:
    typedef izenelib::am::AMIterator<DocidTable> DocidIterator;       //< Iterator on Docid tag.
    typedef izenelib::am::AMIterator<PropertyTable> PropertyIterator; //< Iterator on Property tag.

    /// Open the two leveldb tables associated to tags
    ScdIndexLeveldb(const std::string& docidDatabase, const std::string& propertyDatabase, const bool create) {
        // TODO
        //options.error_if_exists = create;
        //options.create_if_missing = create;

        docidTable.reset(new DocidTable(docidDatabase));
        CHECK(docidTable->open()) << "Cannot open database: " << docidDatabase;

        propertyTable.reset(new PropertyTable(propertyDatabase));
        CHECK(propertyTable->open()) << "Cannot open database: " << propertyDatabase;
    }

    /// Destructor.
    ~ScdIndexLeveldb() {}

    /// Query for Docid table size.
    size_t size() const {
        return docidTable->size();
    }

    /// Insert a new document.
    void insert(const Document<Docid, Property>& document) {
        bool status = docidTable->insert(document.docid, document.offset);
        CHECK(status) << "Cannot insert docid '" << document.docid << "'";

        status = insertProperty(document.property, document.offset);
        CHECK(status) << "Cannot insert property '" << document.property << "'";
    }

    /// Retrieve file offset of a document.
    bool getByDocid(const typename Docid::type& key, offset_type& offset) const {
        return docidTable->get(key, offset);
    }

    /// Retrieve file offset of documents containing the given property.
    bool getByProperty(const typename Property::type& key, offset_list& offsets) const {
        return propertyTable->get(key, offsets);
    }

    /// @return begin iterator on Docid.
    DocidIterator begin() const {
        return DocidIterator(*docidTable);
    }

    /// @return end iterator on Docid.
    DocidIterator end() const {
        return DocidIterator();
    }

    /// @return begin iterator on Property.
    PropertyIterator pbegin() const {
        return PropertyIterator(*propertyTable);
    }

    /// @return end iterator on Property.
    PropertyIterator pend() const {
        return PropertyIterator();
    }

public:
    /// Test only: dump content to output stream.
    void dump(std::ostream& os = std::cout) const {
        os << "* by docid:" << std::endl;
        for (DocidIterator it = begin(), iend = end(); it != iend; ++it) {
            os << it->first << " -> "  << it->second << std::endl;
        }

        os << "* by property:" << std::endl;
        for (PropertyIterator it = pbegin(), iend = pend(); it != iend; ++it) {
            os << it->first << " -> [";
            BOOST_FOREACH(const offset_type& offset, it->second) {
                os << " " << offset << " ";
            }
            os << "]" << std::endl;
        }
    }

private:
    /// Insert a new pair (property, list<offset>).
    inline bool
    insertProperty(const typename Property::type& property, const offset_type& offset) {
        offset_list offsets;
        getByProperty(property, offsets);
        offsets.push_back(offset);
        return propertyTable->insert(property, offsets);
    }

private:
    boost::scoped_ptr<DocidTable> docidTable;
    boost::scoped_ptr<PropertyTable> propertyTable;
};

} /* namespace scd */

#endif /* SCDINDEX_LEVELDB_H */
