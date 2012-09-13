/*
 * File:   ScdIndexDocument.h
 * Author: Paolo D'Apice
 *
 * Created on September 13, 2012, 3:56 PM
 */

#ifndef SCDINDEXDOCUMENT_H
#define	SCDINDEXDOCUMENT_H

#include "ScdParser.h"
#include <glog/logging.h>

namespace scd {

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

private:
    /// Docid name (e.g. 'DOCID').
    static const name_type docid_name;
    /// Property name (e.g. 'uuid').
    static const name_type property_name;
};

// set Document::docid_name
template <typename Docid, typename Property, typename traits>
const typename Document<Docid, Property, traits>::name_type
Document<Docid, Property, traits>::docid_name(Docid::value);

// set Document::property_name
template <typename Docid, typename Property, typename traits>
const typename Document<Docid, Property, traits>::name_type
Document<Docid, Property, traits>::property_name(Property::value);

} /* namespace scd */

#endif	/* SCDINDEXDOCUMENT_H */
