#ifndef PROCESS_CONTROLLERS_TOPIC_CONTROLLER_H
#define PROCESS_CONTROLLERS_TOPIC_CONTROLLER_H
/**
 * @file process/controllers/TopicController.h
 * @author Jia Guo
 * @date Created <2011-03-25>
 */
#include "Sf1Controller.h"

#include <string>

namespace sf1r
{

/// @addtogroup controllers
/// @{

/**
 * @brief Controller \b faceted
 *
 * faceted related
 */
class TopicController : public Sf1Controller
{

public:
    /// @brief alias for set_ontology
    void index()
    {
        get_similar();
    }

    void get_similar();

private:
    bool requireTID_();
    uint32_t tid_;
};

/// @}

} // namespace sf1r

#endif
