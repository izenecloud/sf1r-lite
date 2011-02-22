#ifndef SF1V5_DIRECTORY_MANAGER_DIRECTORY_FWD_H
#define SF1V5_DIRECTORY_MANAGER_DIRECTORY_FWD_H
/**
 * @file directory-manager/Directory_fwd.h
 * @author Ian Yang
 * @date Created <2009-10-27 12:03:48>
 * @date Updated <2009-11-04 18:12:15>
 * @brief forward declaration of types
 */

#include <boost/filesystem.hpp>

namespace sf1r {
namespace directory {

namespace bfs = boost::filesystem;

class Directory;
class DirectoryGuard;
class DirectoryRotator;
class DirectoryCookie;
class NamingStrategy;
}} // namespace sf1r::directory

namespace sf1r {
using directory::Directory;
using directory::DirectoryGuard;
using directory::DirectoryRotator;
} // namespace sf1r

#endif // SF1V5_DIRECTORY_MANAGER_DIRECTORY_FWD_H
