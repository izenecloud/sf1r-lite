#include "DistributeFileSys.h"
#include "SuperNodeManager.h"
#include <common/Utilities.h>

#include <glog/logging.h>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

namespace bfs = boost::filesystem;

static const std::string dfs_copy_prefix = "/sf1r_global_data";

namespace sf1r
{

DistributeFileSys::DistributeFileSys()
    :dfs_enabled_(false)
{
}

bool DistributeFileSys::isEnabled()
{
    return false;
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

std::string DistributeFileSys::getFixedCopyPath(const std::string& custom_prefix)
{
    return dfs_copy_prefix + "/" + SuperNodeManager::get()->getClusterId() + "/" + custom_prefix;
}

bool DistributeFileSys::copyToDFS(std::string& in_out_path, const std::string& custom_prefix, bool fixedpath)
{
    static const bfs::directory_iterator end_it = bfs::directory_iterator();
    bfs::path dfs_out_path(getFixedCopyPath(custom_prefix));

    if (!fixedpath)
        dfs_out_path /= bfs::path(boost::lexical_cast<std::string>(Utilities::createTimeStamp()));

    bfs::path dest = bfs::path(getDFSPath(dfs_out_path.string()));
    if (!bfs::exists(dest))
    {
        bfs::create_directories(dest);
    }
    try
    {
        for( bfs::directory_iterator file(in_out_path); file != end_it; ++file )
        {
            bfs::path current(file->path());
            if (!bfs::exists(current))
            {
                continue;
            }
            if(bfs::is_regular_file(current))
            {
                LOG(INFO) << "copying : " << current << " to " << dest;
                bfs::copy_file(current, dest / current.filename(), bfs::copy_option::overwrite_if_exists);
            }
        }
    }
    catch(const std::exception& e)
    {
        LOG(ERROR) << "coping file to dfs failed: " << e.what();
        return false;
    }
    in_out_path = dfs_out_path.string();
    return true;
}

}
