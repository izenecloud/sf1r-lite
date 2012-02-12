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
        if (right.fp.empty()) return false;
        if (left.fp.empty()) return true;

        for (uint32_t i = bit_mask_.size() - 1; i < bit_mask_.size(); i--)
        {
            if ((left.fp[i] & bit_mask_[i]) < (right.fp[i] & bit_mask_[i]))
                return true;
            if ((left.fp[i] & bit_mask_[i]) > (right.fp[i] & bit_mask_[i]))
                return false;
        }
        return false;
    }

    void GetMaskedBits(const std::vector<uint64_t>& raw_bits, std::vector<uint64_t>& masked_bits) const
    {
        if (raw_bits.empty())
        {
            masked_bits.clear();
            return;
        }

        masked_bits.resize(raw_bits.size());
        for (uint32_t i = 0; i < raw_bits.size(); i++)
        {
            masked_bits[i] = raw_bits[i] & bit_mask_[i];
        }
    }

private:
    friend class FpTables;

    std::vector<uint64_t> bit_mask_;
};

}

#endif
