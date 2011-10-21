#ifndef PROCESS_CONTROLLERS_KEYWORDS_CONTROLLER_H
#define PROCESS_CONTROLLERS_KEYWORDS_CONTROLLER_H
/**
 * @file process/controllers/KeywordsController.h
 * @author Ian Yang
 * @date Created <2010-06-01 16:03:32>
 */
#include "Sf1Controller.h"

namespace sf1r
{

/// @addtogroup controllers
/// @{

/**
 * @brief Controller \b keywords
 *
 * Gets list of keywords.
 */
class KeywordsController : public ::izenelib::driver::Controller
{
public:
    enum
    {
        kDefaultCount = 8       /**< Default count of result */
    };

    void index();

    void inject_query_correction();

    void inject_query_recommend();

//     void finish_inject();
};

/// @}

} // namespace sf1r

#endif // PROCESS_CONTROLLERS_KEYWORDS_CONTROLLER_H


