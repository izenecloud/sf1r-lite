/*
 * File:   ScdIndexDocument.h
 * Author: Paolo D'Apice
 *
 * Created on September 13, 2012, 3:56 PM
 */

#ifndef SCDINDEXDOCUMENT_H
#define SCDINDEXDOCUMENT_H

#include "ScdParserTraits.h"
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
            if (it->first == Docid::name()) {
                docid = Docid::convert(it->second);
            } else if (it->first == Property::name()) {
                property = Property::convert(it->second);
            }
        }
    }

    Document(const offset_type o, const docid_type& id, const property_type& p)
            : offset(o), docid(id), property(p) {}

    friend std::ostream& operator<<(std::ostream& os, const Document& d) {
        os << "@" << d.offset
           << "\t<" << Docid::name() << '>' << d.docid
           << "\t<" << Property::name() << '>' << d.property;
        return os;
    }

    bool operator==(const Document& d) const {
        return offset == d.offset
           and docid == d.docid
           and property == d.property;
    }
};

} /* namespace scd */

#endif /* SCDINDEXDOCUMENT_H */
