
#ifndef SF1R_DD_FPTABLE_H_
#define SF1R_DD_FPTABLE_H_


#include "DupTypes.h"
#include "fp_item.h"
#include <string>
#include <vector>

namespace sf1r
{
class FpTable
{
public:

    typedef std::vector<std::pair<int, int> > PermuteType;
    FpTable(const PermuteType& permute):permute_(permute)
    {
    }

    bool operator() (const FpItem& left, const FpItem& right)
    {
        uint64_t int_left = GetBitsValue( left.fp);
        uint64_t int_right = GetBitsValue( right.fp);
        return int_left < int_right;
    }

    inline uint64_t GetBitsValue(const izenelib::util::CBitArray& bitArray) const
    {
        if (bitArray.GetLength()==0) return 0;
        const uint8_t* p = bitArray.GetBuffer();
//     std::cout<<"[WWWW]"<<","<<nStartCount_[0]<<","<<nStartCount_[1]<<std::endl;
        return izenelib::util::CBitArray::GetBitsValue<uint64_t>(p, permute_);
    }

private:
    std::vector<std::pair<int, int> > permute_;

};

}

#endif
