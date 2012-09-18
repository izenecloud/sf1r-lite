/*
 * File:   ScdIndexDocument.h
 * Author: Paolo D'Apice
 *
 * Created on September 13, 2012, 3:56 PM
 */

#ifndef SCDINDEXDOCUMENT_H
#define SCDINDEXDOCUMENT_H

#include "ScdParserTraits.h"
#include <boost/mpl/integral_c.hpp>
#include <boost/mpl/plus.hpp>
#include <glog/logging.h>

namespace scd {

/**
 * @brief SCD document.
 * Each SCD document is identified by its Docid tag and has an offset
 * within the SCD file.
 */
template <typename Docid, typename Property>
struct Document {
    /// Type of docid values.
    typedef typename Docid::type docid_type;
    /// Type of property values.
    typedef typename Property::type property_type;

    offset_type offset;
    docid_type docid;
    property_type property;

    Document() {}

    Document(const offset_type o, SCDDocPtr doc) : offset(o) {
        for (SCDDoc::iterator it = doc->begin(); it != doc->end(); ++it) {
            // store only indexed properties
            if (it->first == docid_name) {
                docid = Docid::convert(it->second);
            } else if (it->first == property_name) {
                property = Property::convert(it->second);
            }
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const Document& d) {
        os << '<' << docid_name << '>' << d.docid << " @ " << d.offset
           << ": <" << property_name << '>' << d.property;
        return os;
    }

    bool operator==(const Document& d) const {
        return offset == d.offset
           and docid == d.docid
           and property == d.property;
    }

private: // TODO move to serialization header
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive& ar, const unsigned int version) {
        if (version != Version::value) {
            throw std::runtime_error("version mismatch");
        }
        ar & offset;
        ar & docid;
        ar & property;
    }

private:
    /// Docid name (e.g. 'DOCID').
    static const PropertyNameType docid_name;
    /// Property name (e.g. 'uuid').
    static const PropertyNameType property_name;
public:
    /// Serialization version.
    typedef boost::mpl::integral_c<
                unsigned,
                boost::mpl::plus<
                    boost::mpl::int_<Docid::hash>,
                    boost::mpl::int_<Property::hash>
                >::value
            > Version;
};

// set Document::docid_name
template <typename Docid, typename Property>
const PropertyNameType
Document<Docid, Property>::docid_name(Docid::name);

// set Document::property_name
template <typename Docid, typename Property>
const PropertyNameType
Document<Docid, Property>::property_name(Property::name);

} /* namespace scd */

namespace boost { namespace serialization {

    /// Define version using tag hashes.
    template <typename Docid, typename Property>
    struct version<scd::Document<Docid, Property> >{
        typedef mpl::int_<scd::Document<Docid, Property>::Version::value> type;
        typedef mpl::integral_c_tag tag;
        BOOST_STATIC_CONSTANT(unsigned int, value = version::type::value);
    };

}} /* namespace boost::serialization */

#endif /* SCDINDEXDOCUMENT_H */
