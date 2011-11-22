/**
 * @file core/directory-manager/Directory.cpp
 * @author Ian Yang
 * @date Created <2009-10-26 14:07:34>
 * @date Updated <2009-11-27 17:28:01>
 */
#include "Time.h"
#include "DirectoryCookie.h"
#include "Directory.h"
#include "DirectoryTraits.h"

#include <util/filesystem.h>

namespace sf1r {
namespace directory {

const std::string Directory::kNotName;
const std::string Directory::kCookieFileName("cookie");

namespace { // {anonymous}
bool notCookieFile(const bfs::path& path)
{
    return path.filename() != Directory::kCookieFileName;
}
} // namespace {anonymous}

boost::shared_ptr<Directory> Directory::create(const bfs::path& path)
{
    // create empty directory
    bfs::remove_all(path);
    bfs::create_directories(path);

    boost::shared_ptr<Directory> dir(new Directory(path));
    // check error while writing cookie
    if (dir)
    {
        if (!dir->flush())
        {
            dir.reset();
        }
    }

    return dir;
}

boost::shared_ptr<Directory> Directory::open(const bfs::path& path)
{
    // not empty dir
    if (bfs::is_directory(path)
            && bfs::directory_iterator(path) != bfs::directory_iterator())
    {
        boost::shared_ptr<Directory> dir(new Directory(path));
        // check error while reading cookie
        if (dir && ! (dir->refresh() && dir->valid()))
        {
            dir.reset();
        }

        return dir;
    }

    return Directory::create(path);
}

bool Directory::validate(const bfs::path& path)
{
    if (bfs::is_directory(path))
    {
        DirectoryCookie cookie(path / kCookieFileName);
        cookie.setValid();
        return cookie.write();
    }

    return false;
}

Directory::Directory(const bfs::path& path)
    : path_(path)
    , cookie_()
{
    if ("." == path_.filename())
    {
        path_.remove_filename();
    }

    cookie_.reset(new DirectoryCookie(path_ / kCookieFileName));
}

Directory::~Directory()
{
    try
    {
        flush();
    }
    catch(...)
    {
    }
}

bool Directory::refresh()
{
    return cookie_ && cookie_->read();
}

bool Directory::flush()
{
    return cookie_ && cookie_->write();
}

bool Directory::valid() const
{
    return cookie_ && cookie_->valid();
}

const std::string Directory::name() const
{
    return path_.filename();
}

const std::string Directory::parentName() const
{
    return cookie_
        ? cookie_->parentName()
        : DirectoryTraits::kNotName;
}

long Directory::getUpdateTime() const
{
    return cookie_ ? cookie_->getUpdateTime() : 0;
}
void Directory::setUpdateTime()
{
    if (cookie_)
    {
        cookie_->setUpdateTime(Time::secondsSinceEpoch());
    }
}

const bfs::path& Directory::path() const
{
    return path_;
}

const std::string Directory::pathString() const
{
    return path_.file_string() + "/";
}

bool Directory::copyFrom(const Directory& d)
{
    if (!cookie_)
    {
        return false;
    }

    if (!d.valid())
    {
        return false;
    }

    // invalidate cookie first, so if copying corrupts, the cookie will show
    // that the directory is invalid.
    if (!cookie_->invalidate())
    {
        return false;
    }

    izenelib::util::recursive_copy_directory_if(
        d.path(), path(), notCookieFile
    );

    // update cookie
    cookie_->setParentName(d.name());
    cookie_->setUpdateTime(d.getUpdateTime());
    cookie_->setValid();
    return flush();
}

DirectoryGuard::DirectoryGuard(Directory* dir)
    : dir_(dir)
    , good_(false)
{
    if (dir_)
    {
        good_ = dir_->valid()
                && dir_->cookie_
                && dir_->cookie_->invalidate();
    }
}

DirectoryGuard::~DirectoryGuard()
{
    if (dir_ && dir_->cookie_ && good_)
    {
        dir_->setUpdateTime();
        dir_->cookie_->setValid();
        dir_->flush();
    }
}

}} // namespace sf1r::directory
