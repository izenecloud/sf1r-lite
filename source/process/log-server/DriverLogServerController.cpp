#include "DriverLogServerController.h"


namespace sf1r
{

using namespace izenelib::driver;
using driver::Keys;

/**
 * @brief sample, to be modified
 *
 * @example
 * {
 *   "uuid":"0123456789abcdef",
 *   "docid_list":
 *   [
 *     "111",
 *     "222",
 *     "333"
 *   ]
 * }
 *
 */
void DriverLogServerController::update_uuid()
{
    // TODO
    Value& input = request()[Keys::uuid];

    std::cout<<"update_uuid : "<<asString(input)<<std::endl;
}

}
