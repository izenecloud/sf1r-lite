#ifndef SF1V5_DIRECTORY_MANAGER_DIRECTORY_H
#define SF1V5_DIRECTORY_MANAGER_DIRECTORY_H
/**
 * @file directory-manager/Directory.h
 * @author Ian Yang
 * @date Created <2009-10-26 11:42:54>
 * @date Updated <2009-11-27 17:27:41>
 */
#include "Directory_fwd.h"

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include <string>

namespace sf1r {
namespace directory {

class Directory : boost::noncopyable
{
    typedef boost::scoped_ptr<DirectoryCookie> DirectoryCookieHandle;

public:
    const static std::string kNotName;
    const static std::string kCookieFileName;

    /**
     * @brief creates a new directory. If the directory pointed by \a path is an
     * existing file or directory, it is removed first.
     * @param path path of the directory
     * @return valid shared pointer to the new directory if successfully,
     *         otherwise return null pointer.
     * @post if the return value dir is valid shared pointer, then must have \c
     * dir->valid().
     *
     * @warn file or directory pointed by \a path will be removed even this
     * function failed to create the new directory
     */
    static boost::shared_ptr<Directory> create(const bfs::path& path);

    /**
     * @brief opens an existing directory.
     * @param path path of the directory
     * @return valid shared pointer to the opened directory if successfully,
     *         otherwise return null pointer.
     * @post if the return value dir is valid shared pointer, then must have \c
     * dir->valid().
     *
     * If the path points to an existing directory, this function tries to open
     * it and check the health. If the path points to nothing, creates a new
     * direcotry. If the path points to an existing file but not a directory,
     * returns null pointer.
     */
    static boost::shared_ptr<Directory> open(const bfs::path& path);

    /**
     * @brief marks a directory valid
     * @return \c true if the directory is marked valid successfully
     */
    static bool validate(const bfs::path& path);

private:
    explicit Directory(const bfs::path& path);

public:
    ~Directory();

    bool refresh();
    bool flush();

    bool valid() const;

    const std::string name() const;
    const std::string parentName() const;

    long getUpdateTime() const;
    void setUpdateTime();

    const bfs::path& path() const;
    const std::string pathString() const;

    bool copyFrom(const Directory& d);

private:
    bfs::path path_;
    DirectoryCookieHandle cookie_;

    friend class DirectoryGuard;
};

struct DirectoryGuard
{
    DirectoryGuard(Directory* dir);
    ~DirectoryGuard();

    bool good() const
    {
        return good_;
    }

    typedef bool (DirectoryGuard::*unspecified_bool_type)() const;
    operator unspecified_bool_type() const
    {
        return &DirectoryGuard::good;
    }

private:
    Directory* dir_;
    bool good_;
};

}} // namespace sf1r::directory

#endif // SF1V5_DIRECTORY_MANAGER_DIRECTORY_H
