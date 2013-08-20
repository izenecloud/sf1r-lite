/**
 * @file FileEventHandler.h
 * @brief An interface class to handle file event.
 */

#ifndef SF1R_FILE_EVENT_HANDLER_H
#define SF1R_FILE_EVENT_HANDLER_H

#include "../inttypes.h"
#include <string>

namespace sf1r
{

class FileEventHandler
{
public:
    virtual ~FileEventHandler() {}

    virtual bool handle(const std::string& fileName, uint32_t mask) = 0;
};

} // namespace sf1r

#endif // SF1R_FILE_EVENT_HANDLER_H
