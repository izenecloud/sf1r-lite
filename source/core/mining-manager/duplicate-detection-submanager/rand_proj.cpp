/**
   @file rand_proj.cpp
   @author Kevin Hu
   @date 2009.11.24
 */
#include "rand_proj.h"

using namespace std;
namespace sf1r
{

void RandProj::operator+=(const RandProj& rp)
{
    for (uint32_t i = 0; i < proj_.length(); i++)
    {
        proj_[i] += rp.at(i);
    }
}

void RandProj::generate_bitarray(izenelib::util::CBitArray& bitArray)
{
    bitArray.ResetAll();
    for (uint32_t i = 0; i < proj_.length(); i++)
    {
        if (proj_.at(i) >= 0)
        {
            bitArray.SetAt(i);
        }
    }
}

void RandProj::generate_signature(std::vector<uint64_t>& signature)
{
    signature.resize((proj_.length() + 63) / 64);
    for (uint32_t i = 0; i < proj_.length(); i++)
    {
        uint32_t j = i >> 6;
        signature[j] <<= 1;
        if (proj_.at(i) >= 0) ++signature[j];
    }
}

}
