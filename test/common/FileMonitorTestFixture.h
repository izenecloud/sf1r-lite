/**
 * @file FileMonitorTestFixture.h
 * @brief fixture to test FileMonitor
 */

#ifndef SF1R_FILE_MONITOR_TEST_FIXTURE_H
#define SF1R_FILE_MONITOR_TEST_FIXTURE_H

#include <common/file-monitor/FileMonitor.h>

namespace sf1r
{

class FileMonitorTestFixture : public FileMonitor
{
public:
    FileMonitorTestFixture();
    ~FileMonitorTestFixture();

    virtual bool process(const std::string& fileName, uint32_t mask)
    {
        actualFileName_ = fileName;
        actualMask_ = mask;
        return true;
    }

public:
    const std::string dirName_;

    std::string actualFileName_;

    uint32_t actualMask_;
};

} // namespace sf1r

#endif // SF1R_FILE_MONITOR_TEST_FIXTURE_H
