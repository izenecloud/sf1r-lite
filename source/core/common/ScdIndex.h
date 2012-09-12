/*
 * File:   ScdIndex.h
 * Author: Paolo D'Apice
 *
 * Created on September 11, 2012, 12:02 PM
 */

#ifndef SCDINDEX_H
#define	SCDINDEX_H

#include "ScdParser.h"
#include "ScdIndexSerializer.h"
#include <util/ustring/UString.h>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/serialization/map.hpp>

namespace scd {

namespace mi = boost::multi_index;

/// Traits for the SCD document.
struct DocumentTraits {
    typedef std::string name_type;
    typedef izenelib::util::UString value_type;
    typedef long offset_type;
};

/**
 * @brief SCD document.
 * Each SCD document is identified by its DOCID and has an offset
 * within the SCD file.
 */
template <typename traits = DocumentTraits>
struct Document {
    typedef typename traits::name_type name_type;
    typedef typename traits::value_type value_type;
    typedef typename traits::offset_type offset_type;

    offset_type offset;
    std::map<name_type, value_type> properties; // TODO: unordered map

    Document() {}
    Document(const offset_type o, SCDDocPtr doc) : offset(o) {
        for (SCDDoc::iterator it = doc->begin(); it != doc->end(); ++it) {
            properties.insert(std::make_pair(it->first, it->second));
        }
    }

    inline value_type id() const {
        return properties.at("DOCID");
    }

    friend std::ostream& operator<<(std::ostream& os, const Document& d) {
        os << d.id() << " @ " << d.offset;
        return os;
    }

    bool operator==(const Document& d) const {
        return offset == d.offset and properties.size() == d.properties.size()
           and std::equal(properties.begin(), properties.end(), d.properties.begin());
    }

private: // serialization
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive& ar, const unsigned version) {
        ar & offset;
        ar & properties;
    }
};

/// Tag for DOCID property.
struct docid {};

/**
 * @brief SCD multi-index container
 */
class ScdIndex {
    /// Multi-indexed container.
    typedef boost::multi_index_container <
        Document<>,
        mi::indexed_by<
            mi::ordered_unique< // TODO: hashed_unique
                mi::tag<docid>,
                BOOST_MULTI_INDEX_CONST_MEM_FUN(Document<>, Document<>::value_type, id)
            >
        >
    > ScdIndexContainer;

    /// DOCID index.
    typedef mi::index<ScdIndexContainer, docid>::type DocidIndex;

public:
    /// DOCID iterator
    typedef DocidIndex::iterator docid_iterator;

    ScdIndex() {}
    ~ScdIndex() {}

    /**
     * Build an index on an SCD file.
     * @param path The full path to the SCD file.
     * @return A pointer to an instance of ScdIndex.
     */
    static ScdIndex* build(const std::string& path);

    /// @return The size of the index.
    size_t size() const {
        return container.size();
    }

    /// Retrieve tagged content.
    template <typename Tag, typename Type>
    typename mi::index<ScdIndexContainer, Tag>::type::iterator
    find(const Type& key) const {
        // get a reference to the index tagged by Tag
        const typename mi::index<ScdIndexContainer, Tag>::type& index = container.get<Tag>();
        return index.find(key);
    }

    /// Retrieve tagged begin iterator.
    template <typename Tag>
    typename mi::index<ScdIndexContainer, Tag>::type::iterator
    begin() const {
        const typename mi::index<ScdIndexContainer, Tag>::type& index = container.get<Tag>();
        return index.begin();
    }

    /// Retrieve tagged end iterator.
    template <typename Tag>
    typename mi::index<ScdIndexContainer, Tag>::type::iterator
    end() const {
        const typename mi::index<ScdIndexContainer, Tag>::type& index = container.get<Tag>();
        return index.end();
    }

public: // serialization

    /// Save index to file.
    void save(const std::string& filename) const {
        serializer(filename, container);
    }

    /// Load index from file.
    void load(const std::string& filename) {
        deserializer(filename, container);
    }

private:
    ScdIndexContainer container;
    save_txt serializer;
    load_txt deserializer;
};

} /* namespace scd */

#endif	/* SCDINDEX_H */

