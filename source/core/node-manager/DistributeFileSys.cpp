#include "DistributeFileSys.h"

#include <glog/logging.h>
#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;

namespace sf1r
{

DistributeFileSys::DistributeFileSys()
    :dfs_enabled_(false)
{
}

bool DistributeFileSys::isEnabled()
{
    return dfs_enabled_ && !dfs_mount_dir_.empty();
}

void DistributeFileSys::enableDFS(const std::string& mount_dir, const std::string& dfs_local_root)
{
    dfs_enabled_ = true;
    dfs_mount_dir_ = mount_dir;
    dfs_local_root_ = dfs_local_root;
}

std::string DistributeFileSys::getDFSPath(const std::string& dfs_location)
{
    const static std::string empty_path;
    if (!dfs_enabled_ || dfs_mount_dir_.empty())
    {
        LOG(INFO) << "dfs is not enabled.";
        return empty_path;
    }
    return dfs_mount_dir_ + "/" + dfs_location;
}

std::string DistributeFileSys::getDFSLocalFullPath(const std::string& dfs_location)
{
    const static std::string empty_path;
    if (!dfs_enabled_ || dfs_local_root_.empty())
    {
        LOG(INFO) << "dfs is not enabled.";
        return empty_path;
    }
    return dfs_local_root_ + "/" + dfs_location;
}

}
