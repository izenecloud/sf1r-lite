#ifndef SF1R_NODEMANAGER_DISTRIBUTE_FILESYS_H
#define SF1R_NODEMANAGER_DISTRIBUTE_FILESYS_H

#include <string>
#include <util/singleton.h>
#include <boost/shared_ptr.hpp>

namespace sf1r
{

class DistributeFileSys
{
public:
    static DistributeFileSys* get()
    {
        return ::izenelib::util::Singleton<DistributeFileSys>::get();
    }

    DistributeFileSys();
    bool isEnabled();
    void enableDFS(const std::string& mount_dir, const std::string& dfs_local_root);
    std::string getDFSPath(const std::string& dfs_location);
    std::string getDFSLocalFullPath(const std::string& dfs_location);

private:
    bool dfs_enabled_;
    std::string dfs_mount_dir_;
    std::string dfs_local_root_;
};

}

#endif
