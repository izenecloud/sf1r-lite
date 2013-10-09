#ifndef PINYIN_H
#define PINYIN_H

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <vector>

namespace sf1r
{
namespace Recommend
{
typedef boost::function<void(const std::string&, std::vector<std::string>&)> PinYinConverter;
PinYinConverter* getPinYinConverter();
}
}

#endif
