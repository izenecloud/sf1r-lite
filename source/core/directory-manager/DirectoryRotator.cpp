/**
 * @file core/directory-manager/DirectoryRotator.cpp
 * @author Ian Yang
 * @date Created <2009-10-27 17:20:44>
 * @date Updated <2009-11-04 18:22:37>
 */
#include "Directory.h"
#include "DirectoryRotator.h"

#include <boost/next_prior.hpp>

#include <algorithm>

namespace sf1r {
namespace directory {

const DirectoryRotator::DirectoryHandle
DirectoryRotator::kEmptyDirectoryHandle_;

DirectoryRotator::DirectoryRotator(std::size_t capability)
: directories_(capability)
{}

bool DirectoryRotator::appendDirectory(const bfs::path& dirPath,
                                       bool truncate)
{
    if (directories_.full())
    {
        return false;
    }

    DirectoryHandle dir =
        truncate ? Directory::create(dirPath) : Directory::open(dirPath);

    if (dir)
    {
        directories_.push_back(dir);
        return true;
    }

    return false;
}

const DirectoryRotator::DirectoryHandle&
DirectoryRotator::currentDirectory() const
{
    if (!directories_.empty())
    {
        return directories_.front();
    }

    return kEmptyDirectoryHandle_;
}

const DirectoryRotator::DirectoryHandle&
DirectoryRotator::nextDirectory() const
{
    if (directories_.size() > 1)
    {
        return directories_[1];
    }

    return currentDirectory();
}

void DirectoryRotator::rotate()
{
    if (directories_.size() > 1)
    {
        std::rotate(directories_.begin(),
                    boost::next(directories_.begin()),
                    directories_.end());
    }
}

void DirectoryRotator::rotateToNewest()
{
    Container::iterator newest = std::max_element(
        directories_.begin(),
        directories_.end(),
        &sortByUpdateTime_
    );
    std::rotate(
        directories_.begin(),
        newest,
        directories_.end()
    );
}

bool DirectoryRotator::sortByUpdateTime_(const DirectoryHandle& a,
                                         const DirectoryHandle& b)
{
    // we ensure that handle is not null when inserting
    BOOST_ASSERT(a && b);
    return a->getUpdateTime() < b->getUpdateTime();
}

}} // namespace sf1r::directory
