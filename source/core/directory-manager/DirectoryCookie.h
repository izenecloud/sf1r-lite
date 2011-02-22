#ifndef SF1V5_DIRECTORY_MANAGER_DIRECTORY_COOKIE_H
#define SF1V5_DIRECTORY_MANAGER_DIRECTORY_COOKIE_H
/**
 * @file core/directory-manager/DirectoryCookie.h
 * @author Ian Yang
 * @date Created <2009-10-26 17:50:03>
 * @date Updated <2009-10-27 16:16:37>
 * @brief internal header managing cookie reading/writing
 */
#include <directory-manager/Directory_fwd.h>

#include <fstream>
#include <string>

namespace sf1r {
namespace directory {

class DirectoryCookie
{
    static const std::string kMagicCode;
    static const std::string kTrue;
    static const std::string kFalse;

public:
    explicit DirectoryCookie(const bfs::path& filePath);

    bool read();
    bool write();

    long getUpdateTime() const;
    void setUpdateTime(long time);

    const std::string& parentName() const;
    void setParentName(const std::string& name);

    bool invalidate();
    bool valid() const;
    void setValid(bool valid = true);

private:
    bool reopenForRead_();
    bool reopenForWrite_();

    std::string path_;

    long updateTime_;
    std::string parentName_;
    bool valid_;

    std::fstream fs_;
};

}} // namespace sf1r::directory

#endif // SF1V5_DIRECTORY_MANAGER_DIRECTORY_COOKIE_H
