/**
 * @file process/controllers/AutoFillController.cpp
 * @author Ian Yang
 * @date Created <2010-05-31 14:13:29>
 */
#include "AutoFillController.h"

#include <common/Keys.h>

#include <mining-manager/auto-fill-submanager/AutoFillSubManager.h>

#include <util/ustring/UString.h>

#include <vector>
#include <string>

namespace sf1r
{

using driver::Keys;
using namespace izenelib::driver;
/**
 * @brief Action \b index. Gets list of logged keywords starting with specified
 * prefix.
 *
 * @section request
 *
 * - @b prefix* (@c String): Specified keywords prefix.
 * - @b limit (@c Uint = @ref kDefaultCount): Limit the count of result in
 *   response. If the limit is larger than @ref kMaxCount, @ref kMaxCount is
 *   used instead.
 *
 * @section response
 *
 * - @b total_count (@c Uint): Total count of logged keywords starting with the
 *   specified prefix.
 * - @b keywords (@c Array): An array of strings. Every string is a logged
 *   keyword starting with the specified prefix.
 *
 * @section Example
 *
 * Request
 * @code
 * {
 *   "prefix": "auto",
 *   "limit": 10
 * }
 * @endcode
 *
 * Response
 * @code
 * {
 *   "header": {"success": true},
 *   "keywords": [
 *     "autocad",
 *     "autodesk",
 *     "autoexec",
 *     "automatic",
 *     "automobile"
 *   ]
 * }
 * @endcode
 */
void AutoFillController::index()
{
    Value::StringType prefix = asString(request()[Keys::prefix]);
    Value::UintType limit = asUintOr(request()[Keys::limit], kDefaultCount);
    if (limit > kMaxCount)
    {
        limit = kMaxCount;
    }

    if (prefix.empty())
    {
        response().addError("Require field prefix in request.");
        return;
    }

    izenelib::util::UString query(prefix, izenelib::util::UString::UTF_8);
    std::vector<izenelib::util::UString> result;
    AutoFillSubManager::get()->getAutoFillList(query, result);

    Value::UintType count = result.size();
    if (limit < count)
    {
        count = limit;
    }

    response()[Keys::total_count] = result.size();

    // set resource
    Value& keywords = response()[Keys::keywords];
    std::string convertBuffer;
    for (Value::UintType i = 0; i < count; ++i)
    {
        result[i].convertString(convertBuffer, izenelib::util::UString::UTF_8);
        keywords() = convertBuffer;
    }
}

} // namespace sf1r
