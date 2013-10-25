/**
 * @file ByteSizeParser.h
 * @brief It converts between byte size and human readable string.
 */

#ifndef SF1R_BYTE_SIZE_PARSER_H
#define SF1R_BYTE_SIZE_PARSER_H

#include "inttypes.h"
#include <string>
#include <map>
#include <limits>

namespace sf1r
{

class ByteSizeParser
{
public:
    static ByteSizeParser* get();

    ByteSizeParser();

    template <typename IntT>
    IntT parse(const std::string& str) const;

    typedef uint64_t byte_size_t;

    std::string format(byte_size_t value) const;

private:
    byte_size_t parseImpl_(const std::string& str) const;

    void checkAdd_(byte_size_t& a, byte_size_t b, const std::string& str) const;

    void checkMultiply_(byte_size_t& a, byte_size_t b, const std::string& str) const;

    void overflow_(const std::string& str) const;

private:
    typedef std::map<std::string, byte_size_t> UnitSizeMap;
    UnitSizeMap unitSizeMap_;

    typedef std::map<byte_size_t, std::string> SizeUnitMap;
    SizeUnitMap sizeUnitMap_;
};

template <typename IntT>
IntT ByteSizeParser::parse(const std::string& str) const
{
    byte_size_t result = parseImpl_(str);
    byte_size_t maxAllowed = std::numeric_limits<IntT>::max();

    if (result > maxAllowed)
    {
        overflow_(str);
    }

    return result;
}

} // namespace sf1r

#endif // SF1R_BYTE_SIZE_PARSER_H
