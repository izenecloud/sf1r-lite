#ifndef UTILITY_H
#define UTILITY_H

#include <util/ustring/UString.h>

#include <string>
#include <time.h>

using namespace std;

namespace sf1r{

class Utilities
{
public:
    Utilities(){}
    ~Utilities(){}
public:
    static int64_t convertDate(const izenelib::util::UString& dataStr, const izenelib::util::UString::EncodingType& encoding, izenelib::util::UString& outDateStr);
    static int64_t convertDate(const std::string& dataStr);
    static std::string toLower(const std::string& str);
    static std::string toUpper(const std::string& str);
};

}

#endif
