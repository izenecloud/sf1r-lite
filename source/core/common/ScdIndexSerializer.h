/* 
 * File:   ScdIndexSerializer.h
 * Author: Paolo D'Apice
 *
 * Created on September 12, 2012, 9:43 AM
 */

#ifndef SCDINDEXSERIALIZER_H
#define	SCDINDEXSERIALIZER_H

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <fstream>

namespace scd {

namespace ba = boost::archive;

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

/// Function object loading from a text archive.
typedef load_impl<ba::text_iarchive> load_txt;
/// Function object saving into a text archive.
typedef save_impl<ba::text_oarchive> save_txt;

/// Function object loading from a binary archive.
typedef load_impl<ba::binary_iarchive> load_bin;
/// Function object saving into a binary archive.
typedef save_impl<ba::binary_oarchive> save_bin;

} /* namespace scd */

#endif	/* SCDINDEXSERIALIZER_H */