#ifndef PROCESS_CONTROLLERS_STATUS_CONTROLLER_H
#define PROCESS_CONTROLLERS_STATUS_CONTROLLER_H
/**
 * @file process/controllers/StatusController.h
 * @author Ian Yang
 * @date Created <2010-07-07 10:06:17>
 */
#include "Sf1Controller.h"

namespace sf1r
{
class IndexTaskService;

/// @addtogroup controllers
/// @{

/**
 * @brief Controller \b status
 *
 * Gets status of SIA and MIA
 */
class StatusController : public Sf1Controller
{
public:
    StatusController();
    void index();
    void get_distribute_status();

protected:
    virtual bool checkCollectionService(std::string& error);

private:
    IndexTaskService* indexTaskService_;
};

/// @}

} // namespace sf1r

#endif // PROCESS_CONTROLLERS_STATUS_CONTROLLER_H
