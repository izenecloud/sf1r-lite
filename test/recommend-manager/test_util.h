///
/// @file test_util.h
/// @brief util functions to test recommendation
/// @author Jun Jiang
/// @date Created 2012-01-31
///

#ifndef RECOMMEND_TEST_UTIL_H
#define RECOMMEND_TEST_UTIL_H

#include <string>
#include <vector>
#include <sstream>
#include <cstdlib> // rand()

#include <boost/filesystem.hpp>

namespace sf1r
{

/**
 * Split @p str into each item id, and append them into @p items.
 * @param str source string, such as "1 2 3"
 * @param items stores each item id, such as [1, 2, 3]
 */
template<typename ContainerType>
void split_str_to_items(const std::string& str, ContainerType& items)
{
    std::istringstream iss(str);
    typename ContainerType::value_type itemId;

    while (iss >> itemId)
    {
        items.push_back(itemId);
    }
}

/**
 * Generate random item ids, and combine them into string.
 * @param itemCount how many item ids to generate
 * @return the string containing each item ids, such as "12 5 7"
 */
inline std::string generate_rand_items_str(int itemCount)
{
    std::ostringstream oss;
    for (int i = 0; i < itemCount; ++i)
    {
        oss << std::rand() << " ";
    }

    return oss.str();
}

/**
 * Create an empty directory.
 * @param dir the directory path
 */
inline void create_empty_directory(const std::string& dir)
{
    boost::filesystem::remove_all(dir);
    boost::filesystem::create_directories(dir);
}

} // namespace sf1r

#endif //RECOMMEND_TEST_UTIL_H
