/*
 * File:   ScdIndexTableSerializer.h
 * Author: Paolo D'Apice
 *
 * Created on September 24, 2012, 5:07 PM
 */

#ifndef SCDINDEX_TABLE_SERIALIZER_H
#define SCDINDEX_TABLE_SERIALIZER_H

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <sstream>
#include <vector>

namespace scd {

namespace ba = boost::archive;

/// Save a list into an Archive.
template <typename OutputArchive>
struct save_impl {
    template <typename Type>
    void operator()(const std::vector<Type>& list, std::string& destination) const {
        std::ostringstream os;
        OutputArchive oa(os);
        oa << list;
        destination.assign(os.str());
    }
};

/// Load a list from an Archive.
template <typename InputArchive>
struct load_impl {
    template <typename Type>
    void operator()(const std::string& source, std::vector<Type>& list) const {
        std::istringstream is(source);
        InputArchive ia(is);
        ia >> list;
    }
};

/// Function object loading from a binary archive.
typedef load_impl<ba::binary_iarchive> load_bin;
/// Function object saving into a binary archive.
typedef save_impl<ba::binary_oarchive> save_bin;

} /* namespace scd */

#endif /* SCDINDEX_TABLE_SERIALIZER_H */
