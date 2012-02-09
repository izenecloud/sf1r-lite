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
    FpTable()
    {
    }

    explicit FpTable(const std::vector<uint64_t>& bit_mask)
        : bit_mask_(bit_mask)
    {
    }

    void resetBitMask(const std::vector<uint64_t>& bit_mask)
    {
        bit_mask_ = bit_mask;
    }

    const std::vector<uint64_t>& getBitMask() const
    {
        return bit_mask_;
    }

    bool operator() (const FpItem& left, const FpItem& right) const
    {
        for (uint32_t i = 0; i < bit_mask_.size(); i++)
        {
            if ((left.fp[i] & bit_mask_[i]) < (right.fp[i] & bit_mask_[i]))
                return true;
            if ((left.fp[i] & bit_mask_[i]) > (right.fp[i] & bit_mask_[i]))
                return false;
        }
        return false;
    }

private:
    friend class FpTables;

    std::vector<uint64_t> bit_mask_;
};

}

#endif
