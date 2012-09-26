/*
 * File:   uint128.h
 * Author: Paolo D'Apice
 *
 * Created on September 18, 2012, 11:35 AM
 */

#ifndef UINT_128_WRAPPER
#define UINT_128_WRAPPER

#include "Utilities.h"
#include <boost/serialization/binary_object.hpp>

/// Serializable wrapper for uint128_t.
struct uint128 {
    uint128_t value;

    uint128() {};

    uint128(const uint128_t& v) {
        value = v;
    }

    bool operator==(const uint128& other) const {
        return value == other.value;
    }

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar & boost::serialization::make_binary_object(&value, sizeof(value));
    }
};

// hash needed by Boost.MultiIndex
inline size_t hash_value(const uint128& t) {
    return static_cast<size_t>(t.value);
}

// operator<< should be in the global namespace
std::ostream& operator<<(std::ostream& os, const uint128& in) {
    os << static_cast<unsigned long>(in.value);
    return os;
}

/// Convert a izenelib::util::UString into uint128.
inline uint128
str2uint(const izenelib::util::UString& in) {
    return uint128(sf1r::Utilities::md5ToUint128(in));
}

/// Convert a std::string into uint128.
inline uint128
str2uint(const std::string& in) {
    // must use izenelib::util::UString with UTF-8
    return uint128(sf1r::Utilities::md5ToUint128(
        izenelib::util::UString(in, izenelib::util::UString::UTF_8)
    ));
}

/// Convert a uint128 into a std::string.
inline std::string
uint2str(const uint128& in) {
    return sf1r::Utilities::uint128ToMD5(in.value);
}

#endif /* UINT_128_WRAPPER */
