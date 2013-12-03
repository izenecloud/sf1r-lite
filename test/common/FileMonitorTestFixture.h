/**
 * @file FileMonitorTestFixture.h
 * @brief fixture to test FileMonitor
 */

#ifndef SF1R_FILE_MONITOR_TEST_FIXTURE_H
#define SF1R_FILE_MONITOR_TEST_FIXTURE_H

#include <common/file-monitor/FileMonitor.h>
#include <common/file-monitor/FileEventHandler.h>

namespace sf1r
{

class FileMonitorTestFixture : public FileEventHandler
{
public:
    FileMonitorTestFixture();

    virtual bool handle(const std::string& fileName, uint32_t mask);

    void start();

    void resetStatus();

    void checkCommand(
        const std::string& command,
        const std::string& goldFileName,
        uint32_t goldMask);

public:
    FileMonitor monitor_;

    const std::string dirName_;

    std::string actualFileName_;

    uint32_t actualMask_;
};

} // namespace sf1r

#endif // SF1R_FILE_MONITOR_TEST_FIXTURE_H
