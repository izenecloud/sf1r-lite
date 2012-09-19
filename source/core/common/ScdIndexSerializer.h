/*
 * File:   ScdIndexSerializer.h
 * Author: Paolo D'Apice
 *
 * Created on September 12, 2012, 9:43 AM
 */

#ifndef SCDINDEXSERIALIZER_H
#define SCDINDEXSERIALIZER_H

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <fstream>

namespace boost { namespace serialization {

    /// Define version using tag hashes.
    template <typename Docid, typename Property>
    struct version<scd::Document<Docid, Property> >{
        typedef mpl::int_<scd::Document<Docid, Property>::Version::value> type;
        typedef mpl::integral_c_tag tag;
        BOOST_STATIC_CONSTANT(unsigned int, value = version::type::value);
    };

    /// Serialize an SCD document.
    template <typename Archive, typename Docid, typename Property>
    void serialize(Archive& ar, scd::Document<Docid,Property>& doc, const unsigned int version) {
        if (version != scd::Document<Docid,Property>::Version::value) {
            throw std::runtime_error("version mismatch");
        }
        ar & doc.offset;
        ar & doc.docid;
        ar & doc.property;
    }

}} /* namespace boost::serialization */

namespace scd {

namespace ba = boost::archive;
namespace bio = boost::iostreams;

/// Save a MultiIndexContainer into an Archive.
template <typename OutputArchive>
struct save_impl {
    template <typename MultiIndexContainer>
    void operator()(const std::string& filename, const MultiIndexContainer& container) const {
        std::ofstream os(filename.c_str());
        OutputArchive oa(os);
        oa << container;
    }
};

/// Save a MultiIndexContainer into a compressed Archive.
template <typename OutputArchive, typename CompressionFilter>
struct save_compressed_impl {
    template <typename MultiIndexContainer>
    void operator()(const std::string& filename, const MultiIndexContainer& container) const {
        std::ofstream os(filename.c_str());
        bio::filtering_stream<bio::output> fs;
        fs.push(CompressionFilter());
        fs.push(os);
        OutputArchive oa(fs);
        oa << container;
    }
};

/// Load a MultiIndexContainer from an Archive.
template <typename InputArchive>
struct load_impl {
    template <typename MultiIndexContainer>
    void operator()(const std::string& filename, MultiIndexContainer& container) const {
        std::ifstream is(filename.c_str());
        InputArchive ia(is);
        ia >> container;
    }
};

/// Load a MultiIndexContainer from a compressed Archive.
template <typename InputArchive, typename DecompressionFilter>
struct load_compressed_impl {
    template <typename MultiIndexContainer>
    void operator()(const std::string& filename, MultiIndexContainer& container) const {
        std::ifstream is(filename.c_str());
        bio::filtering_stream<bio::input> fs;
        fs.push(DecompressionFilter());
        fs.push(is);
        InputArchive ia(fs);
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

/// Function object loading from a gzip archive.
typedef load_compressed_impl<ba::binary_iarchive, bio::gzip_decompressor> load_gz;
/// Function object saving into a gzip archive.
typedef save_compressed_impl<ba::binary_oarchive, bio::gzip_compressor> save_gz;

/// Function object loading from a zip archive.
typedef load_compressed_impl<ba::binary_iarchive, bio::zlib_decompressor> load_zip;
/// Function object saving into a zip archive.
typedef save_compressed_impl<ba::binary_oarchive, bio::zlib_compressor> save_zip;

/// Function object loading from a bzip2 archive.
typedef load_compressed_impl<ba::binary_iarchive, bio::bzip2_decompressor> load_bz2;
/// Function object saving into a bzip2 archive.
typedef save_compressed_impl<ba::binary_oarchive, bio::bzip2_compressor> save_bz2;

} /* namespace scd */

#endif /* SCDINDEXSERIALIZER_H */
