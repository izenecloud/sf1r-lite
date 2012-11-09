#ifndef PROCESS_CONTROLLERS_COMMANDS_CONTROLLER_H
#define PROCESS_CONTROLLERS_COMMANDS_CONTROLLER_H
/**
 * @file process/controllers/CommandsController.h
 * @author Ian Yang
 * @date Created <2010-06-01 10:20:39>
 */
#include <util/driver/Controller.h>
#include "Sf1Controller.h"

#include <string>

namespace sf1r
{
/// @addtogroup controllers
/// @{

/**
 * @brief Controller \b commands
 *
 * Sends commands to cooresponding collection
 */
class CommandsController : public Sf1Controller
{
public:
    CommandsController();

    void index();

    void mining();

    void index_query_log();

    void optimize_index();

private:
    void indexSearch_();

    void indexRecommend_();
};

/// @}
} // namespace sf1r

#endif // PROCESS_CONTROLLERS_COMMANDS_CONTROLLER_H
