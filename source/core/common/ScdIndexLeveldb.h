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
    typedef Document<Docid, Property> DocumentType;

    typedef typename Docid::type DocidType;
    typedef typename Property::type PropertyType;

    typedef izenelib::am::leveldb::Table<DocidType, offset_type> DocidTable;
    typedef izenelib::am::leveldb::Table<PropertyType, offset_list> PropertyTable;

public:
    typedef izenelib::am::AMIterator<DocidTable> DocidIterator;       //< Iterator on Docid tag.
    typedef izenelib::am::AMIterator<PropertyTable> PropertyIterator; //< Iterator on Property tag.

    /// Open the two leveldb tables associated to tags.
    ScdIndexLeveldb(const std::string& docidDatabase, const std::string& propertyDatabase, const bool);

    /// Destructor.
    ~ScdIndexLeveldb();

    /// Query for Docid table size.
    size_t size() const {
        return docidTable->size();
    }

    /// Insert a new document.
    void insert(const DocumentType& document, const bool batch = false) {
        if (batch)
            insertBatch(document);
        else
            insertDocument(document);
    }

    /// Writes any pending batch operation.
    void flush();

    /// Retrieve file offset of a document.
    bool getByDocid(const DocidType& key, offset_type& offset) const {
        return docidTable->get(key, offset);
    }

    /// Retrieve file offset of documents containing the given property.
    bool getByProperty(const PropertyType& key, offset_list& offsets) const {
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

protected:
#if DEBUG_OUTPUT
    /// Dump content to output stream.
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

    /// Dump buffer content to output stream.
    void dumpBuffer(std::ostream& os = std::cout) const {
        os << "* docid buffer:" << std::endl;
        typedef std::pair<DocidType, offset_type> docid_pair;
        BOOST_FOREACH(const docid_pair& pair, docidBuffer) {
            os << pair.first << " -> "  << pair.second << std::endl;
        }

        os << "* property buffer:" << std::endl;
        typedef std::pair<PropertyType, offset_list> property_pair;
        BOOST_FOREACH(const property_pair& pair, propertyBuffer) {
            os << pair.first << " -> [";
            BOOST_FOREACH(const offset_type& offset, pair.second) {
                os << " " << offset << " ";
            }
            os << "]" << std::endl;
        }
    }
#endif
private:
    /// Directly insert a new document.
    void insertDocument(const DocumentType& document) {
        bool status = docidTable->insert(document.docid, document.offset);
        CHECK(status) << "Cannot insert docid";

        status = insertProperty(document.property, document.offset);
        CHECK(status) << "Cannot insert property";
    }

    /// Insert a new property.
    bool insertProperty(const PropertyType& property, const offset_type& offset) {
        offset_list offsets;
        getByProperty(property, offsets);
        offsets.push_back(offset);
        return propertyTable->insert(property, offsets);
    }

    /// Insert a new property.
    bool insertProperty(const PropertyType& property, const offset_list& offsets) {
        offset_list stored;
        getByProperty(property, stored);
        std::copy(offsets.begin(), offsets.end(), std::back_inserter(stored));
        return propertyTable->insert(property, stored);
    }

    /// Buffer insertion of a new document until flush().
    void insertBatch(const DocumentType& document) {
        using std::make_pair;
        docidBuffer.insert(make_pair(document.docid, document.offset));
        propertyBuffer[document.property].push_back(document.offset);
    }

private:
    boost::scoped_ptr<DocidTable> docidTable;
    boost::scoped_ptr<PropertyTable> propertyTable;

    std::map<DocidType, offset_type> docidBuffer;
    std::map<PropertyType, offset_list> propertyBuffer;
};

template <typename Docid, typename Property>
ScdIndexLeveldb<Docid, Property>::ScdIndexLeveldb(const std::string& docidDatabase,
        const std::string& propertyDatabase, const bool create) {
    // TODO: how to set leveldb options?
    //options.error_if_exists = create;
    //options.create_if_missing = create;

    docidTable.reset(new DocidTable(docidDatabase));
    CHECK(docidTable->open()) << "Cannot open database: " << docidDatabase;

    propertyTable.reset(new PropertyTable(propertyDatabase));
    CHECK(propertyTable->open()) << "Cannot open database: " << propertyDatabase;
}

template <typename Docid, typename Property>
ScdIndexLeveldb<Docid, Property>::~ScdIndexLeveldb() {
    if (not docidBuffer.empty() or not propertyBuffer.empty())
        flush();
}

template <typename Docid, typename Property>
void ScdIndexLeveldb<Docid, Property>::flush() {
    bool status;

    typedef std::pair<DocidType, offset_type> docid_pair;
    BOOST_FOREACH(const docid_pair& pair, docidBuffer) {
        status = docidTable->insert(pair.first, pair.second);
        CHECK(status) << "Cannot insert batch docid";
    }
    docidBuffer.clear();

    typedef std::pair<PropertyType, offset_list> property_pair;
    BOOST_FOREACH(const property_pair& pair, propertyBuffer) {
        status = insertProperty(pair.first, pair.second);
        CHECK(status) << "Cannot insert batch property";
    }
    propertyBuffer.clear();
}

} /* namespace scd */

#endif /* SCDINDEX_LEVELDB_H */
