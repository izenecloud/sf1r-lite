/*
 * File:   ScdIndex.h
 * Author: Paolo D'Apice
 *
 * Created on September 11, 2012, 12:02 PM
 */

#ifndef SCDINDEX_H
#define SCDINDEX_H

#include "ScdIndexDocument.h"
#include "ScdIndexTable.h"
#include "ScdIndexTag.h"
#include "ScdParser.h"

namespace scd {

/**
 * @brief SCD multi-index container
 * This class wraps a container with two indeces:
 * - unique index on the Docid tag
 * - non-unique index on the Property tag
 */
template <typename Property = uuid, typename Docid = DOCID>
class ScdIndex {
public:
    /// Alias for the actual type.
    typedef Document<Docid, Property> document_type;

    /**
     * Create a new index with the given leveldb databases.
     * @param path1 Path to the Docid leveldb database.
     * @param path2 Path to the Property leveldb database.
     */
    ScdIndex(const std::string& path1, const std::string& path2)
            : container(path1, path2) {}

    /// Destructor.
    ~ScdIndex() {}

    /**
     * Build an index on an SCD file.
     * @param scdpath The full path to the SCD file.
     * @param path1 The full path to a directory into which save the docid database.
     * @param path2 The full path to a directory into which save the property database.
     * @param flush_count Flush to storage every \c flush_count documents.
     * @return A pointer to an instance of ScdIndex.
     */
    static ScdIndex<Property, Docid>* build(const std::string& scdpath,
            const std::string& path1,
            const std::string& path2,
            const unsigned flush_count = 1e4);

    /// Get the offset of the document having the given Docid.
    bool find(const typename Docid::type& key, offset_type* offset) const {
        return container.getByDocid(key, offset);
    }

    /// Get the offset of the documents having the given Property.
    bool find(const typename Property::type& key, std::vector<offset_type>& offsets) const {
        return container.getByProperty(key, offsets);
    }

    // for tests only
    void dump() const {
        container.dump();
    }

private:
    ScdIndexTable<Property, Docid> container;
};

template<typename Property, typename Docid>
ScdIndex<Property, Docid>*
ScdIndex<Property, Docid>::build(const std::string& path,
        const std::string& path1,
        const std::string& path2,
        const unsigned flush_count) {
    static ScdParser parser;
    typedef ScdParser::iterator iterator;

    CHECK(parser.load(path)) << "Cannot load file: " << path;
    LOG(INFO) << "Building index on: " << path << " ...";

    ScdIndex<Property, Docid>* index = new ScdIndex(path1, path2);
    iterator end = parser.end();
    unsigned count = 1;
    for (iterator it = parser.begin(); it != end; ++it, ++count) {
        SCDDocPtr doc = *it;
        CHECK(doc) << "Document is null";

        DLOG(INFO) << "got document '" << doc->at(0).second << "' @ " << it.getOffset();
        index->container.insert(document_type(it.getOffset(), doc));

        if (count % flush_count == 0) {
            // TODO flush to leveldb
            LOG(INFO) << "Saved " << count << " documents ...";
        }
    }
    LOG(INFO) << "Indexed " << (count - 1) << " documents.";
    return index;
}

} /* namespace scd */

#endif /* SCDINDEX_H */
