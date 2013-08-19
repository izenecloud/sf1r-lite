/**
 * @file KNlpDictMonitor.h
 * @brief This class is used to monitor knlp resource for update;
            It support moniter/update the resource for knlp
 */

#ifndef SF1R_KNLP_DICT_MONITOR_H
#define SF1R_KNLP_DICT_MONITOR_H

#include "FileMonitor.h"
#include <boost/thread/thread.hpp> 
#include <util/singleton.h>
#include "../ResourceManager.h"
namespace sf1r
{
class KNlpDictMonitor : public FileMonitor
{
public:
    static KNlpDictMonitor* get()
    {
        return izenelib::util::Singleton<KNlpDictMonitor>::get();
    }

    KNlpDictMonitor()
    {
        hasMonitor_ = false;
    }

    virtual bool process(const std::string& fileName, uint32_t mask)
    {
        if (IN_ATTRIB & mask)
        {
            boost::shared_ptr<KNlpWrapper> knlpWrapper(new KNlpWrapper(dictDir_));
            if (!knlpWrapper->loadDictFiles()) return false;
            KNlpResourceManager::setResource(knlpWrapper);
        }
        return true;
    }

    void start(const std::string& dictDir)
    {
        dictDir_ = dictDir;
        if (!hasMonitor_)
        {
            std::string fileName = dictDir_ + "/z.update.time";
            uint32_t mask = IN_ATTRIB;
            addWatch(fileName, mask);
            
            monitor();
            hasMonitor_ = true;
        }
    }

private:
    std::string dictDir_;
    bool hasMonitor_;
};
}

#endif ///SF1R_KNLP_DICT_MONITOR_H