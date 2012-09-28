/*
 * File:   ScdIndexUtils.hpp
 * Author: Paolo D'Apice
 *
 * Created on September 27, 2012, 10:16 AM
 */

#ifndef SCD_INDEX_UTILS_HPP
#define SCD_INDEX_UTILS_HPP

#include "Command.hpp"
#include "ScdBuilder.h"
#include "common/ScdParserTraits.h"
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/test/test_tools.hpp>
#include <iostream>

namespace fs = boost::filesystem;

/// Print an SCD document.
std::ostream& operator<<(std::ostream& os, const SCDDoc& doc) {
    BOOST_FOREACH(const FieldPair& pair, doc) {
        os << '<' << pair.first << '>' << pair.second << '\n';
    }
    return os;
}

namespace test {

/// Create a temporary directory.
fs::path createTempDir(const std::string& name, const bool empty = false) {
    fs::path dir = fs::temp_directory_path()/name;
    if (empty) fs::remove_all(dir);
    fs::create_directories(dir);
    return dir;
}

/// Create a temporary file.
fs::path createTempFile(const std::string& name) {
    fs::path file = fs::temp_directory_path()/name;
    fs::remove_all(file);
    BOOST_REQUIRE(not fs::exists(file));
    return file;
}

inline std::string getDocid(unsigned in) {
    return boost::str(boost::format("%032d") % in);
}

inline std::string getUuid(unsigned in) {
    return boost::str(boost::format("%032d") % (in*in/2));
}

inline std::string getTitle(unsigned in) {
    std::string title("Title ");
    if (in % 2 == 0)
        title.append("T");
    else
        title.append(boost::lexical_cast<std::string>(in));
    return title;
}

/// Create a sample SCD file.
fs::path createScd(const std::string& name,
        const unsigned size, const unsigned start = 1) {
    std::cout << "Creating temporary SCD file with "
              << size << " documents ..." << std::endl;

    fs::path file = fs::temp_directory_path()/name;
    fs::path parent = file.parent_path();
    if (not fs::exists(parent)) fs::create_directories(parent);
    BOOST_REQUIRE(fs::exists(parent));
    if (fs::exists(file)) fs::remove(file);
    BOOST_REQUIRE(not fs::exists(file));

    ScdBuilder scd(file);
    for (unsigned i = start; i < start + size; ++i) {
        scd("DOCID") << getDocid(i);
        scd("Title") << getTitle(i);
        scd("uuid") << getUuid(i);
    }
    BOOST_REQUIRE(fs::exists(file));
    return file;
}

/// Get size of leveldb database.
void databaseSize(const fs::path& p1, const fs::path& p2) {
    size_t s1, s2;
    Command("du " + p1.string() + " | awk '{ print $1 }'").stream() >> s1;
    Command("du " + p2.string() + " | awk '{ print $1 }'").stream() >> s2;
    std::cout << "Database size\n"
              << "\t" << p1.string() << ": " << s1 << " KiB\n"
              << "\t" << p2.string() << ": " << s2 << " KiB\n"
              << "\ttotal: " << s1 + s2 << " KiB" << std::endl;
}

} /* namespace test */

#endif /* SCD_INDEX_UTILS_HPP */
