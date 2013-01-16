#ifndef SF1R_NODEMANAGER_DISTRIBUTE_FILESYNCMGR_H
#define SF1R_NODEMANAGER_DISTRIBUTE_FILESYNCMGR_H

#include <string>
#include <util/singleton.h>
#include <vector>

namespace sf1r
{

class DistributeFileSyncMgr
{
public:
    static DistributeFileSyncMgr* get()
    {
        return ::izenelib::util::Singleton<DistributeFileSyncMgr>::get();
    }

    void getNewestReqLog(uint32_t start_from, const std::string& savepath);
    void getNewestSCDFileList(std::vector<std::string>& filelist);
    bool getFileFromPrimary(const std::string& filepath);
};

}

#endif
