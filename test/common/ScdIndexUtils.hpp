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
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/test/test_tools.hpp>
#include <iostream>

namespace fs = boost::filesystem;

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
        scd("DOCID") << boost::format("%032d") % i;
        if (i % 2 == 0)
            scd("Title") << "Title T";
        else
            scd("Title") << "Title " << i;
        scd("uuid") << boost::format("%032d") % (i*i/2);
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
