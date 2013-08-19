/**
 * @file KNlpDictMonitor.h
 * @brief This class is used to monitor knlp resource for update;
 *        It support moniter/update the resource for knlp.
 */

#ifndef SF1R_KNLP_DICT_MONITOR_H
#define SF1R_KNLP_DICT_MONITOR_H

#include <common/file-monitor/FileMonitor.h>
#include <common/file-monitor/FileEventHandler.h>

namespace sf1r
{

class KNlpDictMonitor : public FileEventHandler
{
public:
    static KNlpDictMonitor* get();

    KNlpDictMonitor();

    virtual bool handle(const std::string& fileName, uint32_t mask);

    void start(const std::string& dictDir);

private:
    FileMonitor monitor_;

    std::string dictDir_;

    bool isStart_;
};

} // namespace sf1r

#endif // SF1R_KNLP_DICT_MONITOR_H
