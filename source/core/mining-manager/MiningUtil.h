///
/// @file   MiningUtil.h
/// @brief  Provide some useful function for MIA
/// @author Jia Guo
/// @date   2009-12-07
///
#ifndef MININGUTIL_H_
#define MININGUTIL_H_
#include <common/inttypes.h>
#include <vector>
namespace sf1r
{

class MiningUtil
{

private:
    MiningUtil(){}
public:
    static void encodeIntegers( const std::vector<uint32_t>& ints,char* data, uint32_t& len);
};
}
#endif
