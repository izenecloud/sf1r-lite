/* 
 * File:   ScdIndex.h
 * Author: Paolo D'Apice
 *
 * Created on September 11, 2012, 12:02 PM
 */

#ifndef SCDINDEX_H
#define	SCDINDEX_H

#include <util/ustring/UString.h>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

namespace ba = boost::archive;
namespace mi = boost::multi_index;

namespace scd { 

/// Traits for the SCD document.
struct DocumentTraits {
    typedef typename izenelib::util::UString docid_type;
    typedef long offset_type;
};

/**
 * @brief SCD document.
 * Each SCD document is identified by its DOCID and has an offset 
 * within the SCD file.
 */
template <typename traits = DocumentTraits>
struct Document {
    typedef typename traits::docid_type docid_type;
    typedef typename traits::offset_type offset_type;

    docid_type id;
    offset_type offset;
    
    Document() {}
    Document(const docid_type& docid, const offset_type os) : id(docid), offset(os) {}

    // TODO: properties
    
    friend std::ostream& operator<<(std::ostream& os, const Document& d) {
        os << d.id << "->" << d.offset;
        return os;
    }

    bool operator==(const Document& d) const {
        return id == d.id and offset == d.offset;
    }
    
private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive& ar, const unsigned version) {
        ar & id;
        ar & offset;
    }
};

/// Tag for DOCID property.
struct docid {};

/// Multi-indexed container.
typedef boost::multi_index_container <
    Document<>,
    mi::indexed_by<
        mi::ordered_unique< // TODO: hashed_unique
            mi::tag<docid>,
            BOOST_MULTI_INDEX_MEMBER(Document<>, Document<>::docid_type, id)
        >
    >
> ScdIndex;

/// Save a MultiIndexContainer into an Output Archive.
template <typename OutputArchive>
struct save_impl {
    template <typename MultiIndexContainer>
    void operator()(const std::string& filename, const MultiIndexContainer& container) const {
        std::ofstream os(filename.c_str());
        OutputArchive oa(os);
        oa << container;
    }
};

/// Load a MultiIndexContainer from an Output Archive.
template <typename InputArchive>
struct load_impl {
    template <typename MultiIndexContainer>
    void operator()(const std::string& filename, MultiIndexContainer& container) const {
        std::ifstream is(filename.c_str());
        InputArchive ia(is);
        ia >> container;
    }
};

/// Retrieve a document by its DOCID.
template <typename Tag, typename MultiIndexContainer>
typename mi::index<MultiIndexContainer, Tag>::type::iterator
get(const MultiIndexContainer& container, const DocumentTraits::docid_type& key) {
    // get a reference to the index tagged by Tag
    const typename mi::index<MultiIndexContainer, Tag>::type& index = mi::get<Tag>(container);
    return index.find(key);
}

/// Function object loading from a text archive.
const load_impl<ba::text_iarchive> load;
/// Function object saving into a text archive.
const save_impl<ba::text_oarchive> save;

/// Function object loading from a binary archive.
const load_impl<ba::binary_iarchive> load_bin;
/// Function object saving into a binary archive.
const save_impl<ba::binary_oarchive> save_bin;

} /* namespace scd */

#endif	/* SCDINDEX_H */

