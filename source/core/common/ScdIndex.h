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
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/serialization/map.hpp>
#include <glog/logging.h>

namespace scd {

namespace mi = boost::multi_index;

namespace {

/// Default traits for the SCD document.
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
template <typename Docid, typename Property, typename traits = DocumentTraits>
struct Document {
    /// Type of property names.
    typedef typename traits::name_type name_type;
    /// Type of property values.
    typedef typename traits::value_type value_type;
    /// Type of offset values.
    typedef typename traits::offset_type offset_type;

    /// Docid name (e.g. 'DOCID').
    static const name_type docid_name;
    /// Property name (e.g. 'uuid').
    static const name_type property_name;
    
    offset_type offset;
    std::map<name_type, value_type> properties; // TODO: unordered map

    Document() {}

    Document(const offset_type o, SCDDocPtr doc) : offset(o) {
        for (SCDDoc::iterator it = doc->begin(); it != doc->end(); ++it) {
            // store only indexed properties
            if (it->first == docid_name or it->first == property_name)
                properties.insert(std::make_pair(it->first, it->second));
        }
        CHECK_EQ(2, properties.size()) << "Wrong number of properties: " << properties.size();
    }

    inline value_type docid() const {
        return properties.at(docid_name);
    }

    inline value_type property() const {
        return properties.at(property_name);
    }

    friend std::ostream& operator<<(std::ostream& os, const Document& d) {
        os << '<' << docid_name << '>' << d.docid() << " @ " << d.offset 
           << ": <" << property_name << '>' << d.property();
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

// set Document::docid_name
template <typename Docid, typename Property, typename traits>
const typename Document<Docid, Property, traits>::name_type
Document<Docid, Property, traits>::docid_name(Docid::value);

// set Document::property_name
template <typename Docid, typename Property, typename traits>
const typename Document<Docid, Property, traits>::name_type
Document<Docid, Property, traits>::property_name(Property::value);

} /* unnamed namespace */

/// Create a new property tag.
#define SCD_INDEX_PROPERTY_TAG(tag_name, tag_value) \
    struct tag_name { static const char value[]; }; \
    const char tag_name::value[] = tag_value;

/// Tag for DOCID property.
SCD_INDEX_PROPERTY_TAG(docid, "DOCID");

/**
 * @brief SCD multi-index container
 * This class wraps a container with two indeces:
 * - unique index on the Docid tag
 * - non-unique index on the Property tag
 */
template <typename Property, typename Docid = docid>
struct ScdIndex {
    typedef Document<Docid, Property> document_type;
private:
    /// Multi-indexed container.
    typedef boost::multi_index_container <
        document_type,
        mi::indexed_by<
            mi::ordered_unique< // TODO: hashed_unique (serialization!)
                mi::tag<Docid>,
                BOOST_MULTI_INDEX_CONST_MEM_FUN(document_type, typename document_type::value_type, docid)
            >,
            mi::ordered_non_unique<
                mi::tag<Property>,
                BOOST_MULTI_INDEX_CONST_MEM_FUN(document_type, typename document_type::value_type, property)
            >
            // TODO: more properties?
        >
    > ScdIndexContainer;

    /// DOCID index.
    typedef typename mi::index<ScdIndexContainer, Docid>::type DocidIndex;
    /// Property index.
    typedef typename mi::index<ScdIndexContainer, Property>::type PropertyIndex;

public:
    /// DOCID iterator.
    typedef typename DocidIndex::iterator docid_iterator;
    /// Property iterator.
    typedef typename PropertyIndex::iterator property_iterator;

    ScdIndex() {}
    ~ScdIndex() {}

    /**
     * Build an index on an SCD file.
     * @param path The full path to the SCD file.
     * @return A pointer to an instance of ScdIndex.
     */
    static ScdIndex<Property, Docid>* build(const std::string& path);

    /// @return The size of the index.
    size_t size() const {
        return container.size();
    }

    /// Count tagged elements.
    template <typename Tag, typename Type>
    size_t count(const Type& key) const {
        // get a reference to the index tagged by Tag
        const typename mi::index<ScdIndexContainer, Tag>::type& index = mi::get<Tag>(container);
        return index.count(key);
    }

    /// Retrieve tagged content.
    template <typename Tag, typename Type>
    typename mi::index<ScdIndexContainer, Tag>::type::iterator
    find(const Type& key) const {
        // get a reference to the index tagged by Tag
        const typename mi::index<ScdIndexContainer, Tag>::type& index = mi::get<Tag>(container);
        return index.find(key);
    }

    /// Retrieve tagged begin iterator.
    template <typename Tag>
    typename mi::index<ScdIndexContainer, Tag>::type::iterator
    begin() const {
        const typename mi::index<ScdIndexContainer, Tag>::type& index = mi::get<Tag>(container);
        return index.begin();
    }

    /// Retrieve tagged end iterator.
    template <typename Tag>
    typename mi::index<ScdIndexContainer, Tag>::type::iterator
    end() const {
        const typename mi::index<ScdIndexContainer, Tag>::type& index = mi::get<Tag>(container);
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

template<typename Property, typename Docid>
ScdIndex<Property, Docid>*
ScdIndex<Property, Docid>::build(const std::string& path) {
    static ScdParser parser;
    typedef ScdParser::iterator iterator;

    CHECK(parser.load(path)) << "Cannot load file: " << path;
    LOG(INFO) << "Building index on: " << path << " ...";

    ScdIndex<Property, Docid>* index = new ScdIndex;
    iterator end = parser.end();
    for (iterator it = parser.begin(); it != end; ++it) {
        SCDDocPtr doc = *it;
        CHECK(doc) << "Document is null";

        DLOG(INFO) << "got document '" << doc->at(0).second << "' @ " << it.getOffset();

        index->container.insert(document_type(it.getOffset(), doc));
        LOG_EVERY_N(INFO, 100) << "Saved " << google::COUNTER << " documents ...";
    }

    LOG(INFO) << "Indexed " << index->size() << " documents.";

    return index;
}

} /* namespace scd */

#endif	/* SCDINDEX_H */
