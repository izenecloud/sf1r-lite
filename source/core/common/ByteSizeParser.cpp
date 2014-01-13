#include "ByteSizeParser.h"
#include <util/singleton.h>
#include <sstream>
#include <stdexcept>
#include <cctype> // isspace, isdigit, toupper
#include <cstdlib> // atof
#include <iomanip> // setprecision

using namespace sf1r;

namespace
{
const std::string kMessageInvalidUnit = "invalid byte unit ";

const ByteSizeParser::byte_size_t kMaxByteSize =
    std::numeric_limits<ByteSizeParser::byte_size_t>::max();
}

ByteSizeParser* ByteSizeParser::get()
{
    return izenelib::util::Singleton<ByteSizeParser>::get();
}

ByteSizeParser::ByteSizeParser()
{
    byte_size_t one = 1;

    unitSizeMap_["B"] = one;
    unitSizeMap_["K"] = unitSizeMap_["KB"] = one << 10;
    unitSizeMap_["M"] = unitSizeMap_["MB"] = one << 20;
    unitSizeMap_["G"] = unitSizeMap_["GB"] = one << 30;
    unitSizeMap_["T"] = unitSizeMap_["TB"] = one << 40;
    unitSizeMap_["P"] = unitSizeMap_["PB"] = one << 50;
    unitSizeMap_["E"] = unitSizeMap_["EB"] = one << 60;

    sizeUnitMap_[one << 10] = "K";
    sizeUnitMap_[one << 20] = "M";
    sizeUnitMap_[one << 30] = "G";
    sizeUnitMap_[one << 40] = "T";
    sizeUnitMap_[one << 50] = "P";
    sizeUnitMap_[one << 60] = "E";
}

ByteSizeParser::byte_size_t ByteSizeParser::parseImpl_(const std::string& str) const
{
    const char* p = str.c_str();
    byte_size_t value = 0;

    for (; std::isspace(*p); ++p) ;

    if (!std::isdigit(*p))
    {
        throw std::runtime_error("invalid format, digit required.");
    }

    for (; std::isdigit(*p); ++p)
    {
        checkMultiply_(value, 10, str);
        checkAdd_(value, *p - '0', str);
    }

    double fraction = 0;
    if (*p == '.')
    {
        fraction = std::atof(p);

        for (++p; std::isdigit(*p); ++p) ;
    }

    std::string unit;
    const char* pUnit = p;
    for (; *p; ++p)
    {
        if (std::isspace(*p)) continue;

        unit += std::toupper(*p);
    }

    if (!unit.empty())
    {
        UnitSizeMap::const_iterator iter = unitSizeMap_.find(unit);

        if (iter == unitSizeMap_.end())
        {
            throw std::runtime_error(kMessageInvalidUnit + pUnit);
        }

        byte_size_t unitSize = iter->second;
        checkMultiply_(value, unitSize, str);

        if (fraction)
        {
            checkAdd_(value, fraction * unitSize, str);
        }
    }

    return value;
}

void ByteSizeParser::checkAdd_(byte_size_t& a, byte_size_t b, const std::string& str) const
{
    if (a > kMaxByteSize - b)
    {
        overflow_(str);
    }

    a += b;
}

void ByteSizeParser::checkMultiply_(byte_size_t& a, byte_size_t b, const std::string& str) const
{
    if (b > 0 && a > kMaxByteSize / b)
    {
        overflow_(str);
    }

    a *= b;
}

void ByteSizeParser::overflow_(const std::string& str) const
{
    throw std::overflow_error("overflowed when converting " + str +
                              " to destination type");
}

std::string ByteSizeParser::format(byte_size_t value) const
{
    SizeUnitMap::const_iterator iter = sizeUnitMap_.upper_bound(value);
    double quotient = value;
    int precision = 0;
    std::string unit;

    if (iter != sizeUnitMap_.begin())
    {
        --iter;

        quotient /= iter->first;
        unit = iter->second;

        if (quotient < 10)
        {
            precision = 1;
        }
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << quotient;
    std::string result = oss.str() + unit;

    return result;
}
