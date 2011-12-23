#include "DriverLogServerController.h"


namespace sf1r
{

using namespace izenelib::driver;
using driver::Keys;

/**
 * @brief Action \b update_cclog
 * The request data of \b update_cclog in json format is expected supported by any action of SF1R controllers,
 * and the \b header field is presented.
 *
 * @example
 * {
 *   "header" : {
 *     "action" : "documents",
 *     "controller" : "visit"
 *   },
 *
 *   # Following by other fields (this is comment)
 * }
 *
 */
void DriverLogServerController::update_cclog()
{
    // TODO
    std::cout<<"got cclog request : "
             <<request().controller()<<" "
             <<request().action()<<std::endl;
}

}
