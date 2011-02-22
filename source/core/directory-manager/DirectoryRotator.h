#ifndef INCLUDE_SF1V5_DIRECTORY_MANAGER_DIRECTORY_ROTATOR_H
#define INCLUDE_SF1V5_DIRECTORY_MANAGER_DIRECTORY_ROTATOR_H
/**
 * @file include/directory-manager/DirectoryRotator.h
 * @author Ian Yang
 * @date Created <2009-10-26 11:32:55>
 * @date Updated <2009-11-04 18:14:31>
 */

#include "Directory_fwd.h"
#include "Directory.h"

#include <boost/circular_buffer.hpp>
#include <boost/shared_ptr.hpp>

namespace sf1r {
namespace directory {

class DirectoryRotator
{
    typedef boost::shared_ptr<Directory> DirectoryHandle;
    typedef boost::circular_buffer<DirectoryHandle> Container;

public:
    explicit DirectoryRotator(std::size_t capacity = 2);

    /**
     * @brief adds one directory at the end of the rotate buffer
     */
    bool appendDirectory(const bfs::path& dirPath,
                         bool truncate = false);

    void setCapacity(std::size_t capacity)
    {
        directories_.set_capacity(capacity);
    }
    std::size_t size() const
    {
        return directories_.size();
    }

    /**
     * @brief tells current directory
     */
    const DirectoryHandle& currentDirectory() const;

    /**
     * @brief tells next directory
     */
    const DirectoryHandle& nextDirectory() const;

    void clearDirectories()
    {
        directories_.clear();
    }
    /**
     * @brief rotate one directory, next directory becomes current.
     */
    void rotate();

    void rotateToNewest();

private:
    Container directories_;

    static bool sortByUpdateTime_(const DirectoryHandle& a,
                                  const DirectoryHandle& b);

    static const DirectoryHandle kEmptyDirectoryHandle_;
};

}} // namespace sf1r::directory

#endif // INCLUDE_SF1V5_DIRECTORY_MANAGER_DIRECTORY_ROTATOR_H
