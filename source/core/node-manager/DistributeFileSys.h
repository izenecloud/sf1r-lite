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
    // return the full path for mounted dfs path, the local mount root will be inserted.
    std::string getDFSPath(const std::string& dfs_location);
    // return the full path for local shard node, node_prefix_path will be inserted.
    std::string getDFSLocalFullPath(const std::string& dfs_location);
    bool copyToDFS(std::string& in_out_path, const std::string& custom_prefix, bool fixedpath = false);
    std::string getFixedCopyPath(const std::string& custom_prefix);

private:
    bool dfs_enabled_;
    std::string dfs_mount_dir_;
    std::string dfs_local_root_;
};

}

#endif
