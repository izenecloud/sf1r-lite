#ifndef PROCESS_CONTROLLERS_COMMANDS_CONTROLLER_H
#define PROCESS_CONTROLLERS_COMMANDS_CONTROLLER_H
/**
 * @file process/controllers/CommandsController.h
 * @author Ian Yang
 * @date Created <2010-06-01 10:20:39>
 */
#include <util/driver/Controller.h>

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
class CommandsController : public ::izenelib::driver::Controller
{
public:
    void index();

    void index_recommend();
    
    void index_query_log();

    void optimize_index();

    bool parseCollection();

private:
    std::string collection_;
};

/// @}
} // namespace sf1r

#endif // PROCESS_CONTROLLERS_COMMANDS_CONTROLLER_H
