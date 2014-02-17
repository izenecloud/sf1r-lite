/**
 * @file TableSwapper.h
 * @brief swap the instances of reader and writer.
 */

#ifndef SF1R_TABLE_SWAPPER_H
#define SF1R_TABLE_SWAPPER_H

#include <cstddef>

namespace sf1r
{

template <class Table>
struct TableSwapper
{
    Table& reader;
    Table writer;

    TableSwapper(Table& r) : reader(r) {}

    void copy(std::size_t newSize)
    {
        writer = reader;
        writer.resize(newSize);
    }

    void swap()
    {
        writer.swap(reader);

        // release the memory of writer
        Table().swap(writer);
    }
};

} // namespace sf1r

#endif // SF1R_TABLE_SWAPPER_H
