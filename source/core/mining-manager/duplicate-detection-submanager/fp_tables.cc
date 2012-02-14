#include "fp_tables.h"

using namespace sf1r;

void FpTables::GenTables(uint32_t f, uint8_t k, uint8_t partition_num, std::vector<FpTable>& table_list)
{
    std::vector<uint8_t> blocks(partition_num, f / partition_num);
    uint8_t remain = f % partition_num;
    for (uint8_t i = 0; i < remain; i++)
    {
        ++blocks[i];
    }
    std::vector<std::vector<std::pair<uint8_t, uint64_t> > > mask_blocks(blocks.size());
    std::pair<uint8_t, uint8_t> start, finish(0, 0);
    for (uint16_t i = 0, count = 0; i < blocks.size(); ++i)
    {
        std::vector<std::pair<uint8_t, uint64_t> >& mask_block = mask_blocks[i];
        count += blocks[i];
        start = finish;
        finish.first = count / 64;
        finish.second = count % 64;

        if (start.first == finish.first)
        {
            mask_block.push_back(std::make_pair(start.first, ((uint64_t(1) << blocks[i]) - 1) << start.second));
            continue;
        }

        mask_block.push_back(std::make_pair(start.first, -(uint64_t(1) << start.second)));
        for (uint8_t j = start.first + 1; j < finish.first; j++)
        {
            mask_block.push_back(std::make_pair(j, uint64_t(-1)));
        }
        if (finish.second)
        {
            mask_block.push_back(std::make_pair(finish.first, (uint64_t(1) << finish.second) - 1));
        }
    }
    //select blocks in C_pn^(pn-k) in blocks
    uint8_t len = partition_num - k;
    //init

    std::vector<uint8_t> vec(len);
    for (uint8_t i = 0; i < len; i++)
    {
        vec[i] = i;
    }

    do
    {
        table_list.push_back(FpTable());
        std::vector<uint64_t>& bit_mask = table_list.back().bit_mask_;
        bit_mask.resize((f + 63) / 64);
        for (uint8_t i = 0; i < len; i++)
        {
            std::vector<std::pair<uint8_t, uint64_t> >::const_iterator it
                = mask_blocks[vec[i]].begin();
            for (; it != mask_blocks[vec[i]].end(); ++it)
                bit_mask[it->first] |= it->second;
        }
    }
    while (permutate_(vec, len - 1, blocks.size()));
}

bool FpTables::permutate_(std::vector<uint8_t >& vec, uint32_t index, uint32_t size)
{
    if (vec.size() <= index) return false;

    uint8_t pos = vec.size() - index;
    if (vec[index] == size - pos)
    {
        return permutate_(vec, index - 1, size);
    }

    ++vec[index];
    for (uint32_t i = index + 1; i < vec.size(); i++ )
    {
        vec[i] = vec[index] + i - index;
    }
    return true;
}
