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
    void enableDFS(const std::string& mount_dir, const std::string& dfs_local_node_root);
    // return the full path for mounted dfs path, the local mount root will be inserted.
    std::string getDFSPathForLocal(const std::string& dfs_location);
    // return the full path for local shard node, node_prefix_path will be inserted.
    std::string getDFSPathForLocalNode(const std::string& dfs_location);
    // copy the files under the in_out_path directory to the DFS, 
    // if fixedpath is not set, a directory will be created with the current time string.
    // and return the saved path for the DFS.
    bool copyToDFS(std::string& in_out_path, const std::string& custom_prefix, bool fixedpath = false);
    // return the file copy path for the DFS.
    // to avoid the same file location in different cluster,
    // the cluster name will be interted to the given path.
    std::string getFixedCopyPath(const std::string& custom_prefix);

private:
    bool dfs_enabled_;
    std::string dfs_mount_dir_;
    std::string dfs_local_node_root_;
};

}

#endif
