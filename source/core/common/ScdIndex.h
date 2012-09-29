/*
 * File:   ScdIndex.h
 * Author: Paolo D'Apice
 *
 * Created on September 11, 2012, 12:02 PM
 */

#ifndef SCDINDEX_H
#define SCDINDEX_H

#include "ScdIndexDocument.h"
#include "ScdIndexIterator.h"
#include "ScdIndexLeveldb.h"
#include "ScdIndexTag.h"
#include "ScdParser.h"

namespace scd {

/**
 * @brief SCD multi-index container
 * This class wraps a container with two indeces allowing to retrieve
 * document offsets within an SCD file:
 * - unique index on the specified Docid tag (default to 'DOCID')
 * - non-unique index on the scpecified Property tag (default to 'uuid')
 */
template <typename Property = uuid, typename Docid = DOCID>
class ScdIndex {
    typedef ScdIndexLeveldb<Docid, Property> ContainerType; //< The actual container type.

    ScdParser parser;
    ContainerType container;

public:
    typedef Document<Docid, Property> DocumentType; //< The actual document type.

    typedef typename Docid::type DocidType;          //< The actual Docid type.
    typedef typename Property::type PropertyType;    //< The actual Property type

    typedef ScdIndexIterator iterator;

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
            const unsigned flush_count = 1e5);

    /**
     * Load an existing index with the given leveldb databases.
     * @param scdpath The full path to the SCD file.
     * @param path1 Path to the Docid leveldb database.
     * @param path2 Path to the Property leveldb database.
     */
    static ScdIndex<Property, Docid>* load(const std::string& scdpath,
            const std::string& path1,
            const std::string& path2);

    /// @return The number of stored documents.
    size_t size() const {
        return container.size();
    }

    /// @return An iterator to the document having the given Docid.
    ScdIndexIterator findDoc(const DocidType& key) {
        offset_type offset;
        bool found = container.getByDocid(key, offset);
        if (found)
            return ScdIndexIterator(&parser, offset);
        return ScdIndexIterator();
    }

    /// @return An iterator to the documents having the requested property value.
    ScdIndexIterator find(const PropertyType& key) {
        offset_list offsets;
        bool found = container.getByProperty(key, offsets);
        if (found)
            return ScdIndexIterator(&parser, offsets);
        return ScdIndexIterator();
    }

private:
    /// Constructor.
    ScdIndex(const std::string& path1, const std::string& path2, const bool create)
            : container(path1, path2, create) {}
};

template<typename Property, typename Docid>
ScdIndex<Property, Docid>*
ScdIndex<Property, Docid>::build(const std::string& path,
        const std::string& path1,
        const std::string& path2,
        const unsigned flush_count) {
    ScdIndex<Property, Docid>* index = new ScdIndex(path1, path2, true);
    CHECK(index->parser.load(path)) << "Cannot load file: " << path;
    LOG(INFO) << "Building index on: " << path << " ...";

    typedef ScdParser::iterator iterator;
    iterator end = index->parser.end();
    unsigned count = 1;
    for (iterator it = index->parser.begin(); it != end; ++it, ++count) {
        SCDDocPtr doc = *it;
        CHECK(doc) << "Document is null";

        DLOG(INFO) << "got document '" << doc->at(0).second << "' @ " << it.getOffset();
        index->container.insert(DocumentType(it.getOffset(), doc), true);

        if (count % flush_count == 0) {
            index->container.flush();
            LOG(INFO) << "Saved " << count << " documents ...";
        }
    }
    index->container.flush();
    LOG(INFO) << "Indexed " << (count - 1) << " documents.";
    return index;
}

template<typename Property, typename Docid>
ScdIndex<Property, Docid>*
ScdIndex<Property, Docid>::load(const std::string& path,
        const std::string& path1,
        const std::string& path2) {
    ScdIndex<Property, Docid>* index = new ScdIndex(path1, path2, false);
    CHECK(index->parser.load(path)) << "Cannot load file: " << path;
    LOG(INFO) << "Loaded index for: " << path << " ...";
    return index;
}

} /* namespace scd */

#endif /* SCDINDEX_H */
