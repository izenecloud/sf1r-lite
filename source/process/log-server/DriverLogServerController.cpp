#include "DriverLogServerController.h"


namespace sf1r
{

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
    std::cout<<"got cclog request : "
             <<request().controller()<<" "
             <<request().action()<<std::endl;

    // TODO: update uuid(docid)

    std::string raw;
    jsonWriter_.write(request().get(), raw);

    std::cout<<raw<<std::endl;
}


}
