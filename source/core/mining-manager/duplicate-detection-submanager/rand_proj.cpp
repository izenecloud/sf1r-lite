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

}
