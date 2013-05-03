/**
 * @file core/common/parsers/ConditionArrayParser.cpp
 * @author Ian Yang
 * @date Created <2010-06-10 20:10:01>
 */
#include "ConditionArrayParser.h"

namespace sf1r {
/**
 * @class ConditionArrayParser
 *
 * The @b conditions field is an array. It specifies filtering conditions on
 * properties. All conditions are combined using AND, i.e., returned result must
 * satisfied all conditions.This conditions can also be a tree-like construtcion, see @b ConditionTreeParser.
 *
 * Every item is an object of following fields:
 * - @b property* (@c String): Which property current condition applies on.
 * - @b operator* (@c String): Which kind operator.
 *   - @c = : Property must equal to the specified value.
 *   - @c <> : Property should not equal to the specified value.
 *   - @c < : Property must be less than the specified value.
 *   - @c <= : Property must be less than or equal to the specified value.
 *   - @c > : Property must be larger than the specified value.
 *   - @c >= : Property must be larger than or equal to the specified value.
 *   - @c between : Property must be in the range inclusively.
 *   - @c in : Property must be equal to any of the list values.
 *   - @c not_in : Property should not be equal to any of the list values.
 *   - @c starts_with : Property must start with the specified prefix.
 *   - @c ends_with : Property must end with the specified suffix.
 *   - @c contains : Property must contains the specified sub-string.
 * - @b value (@c Any): If this is an array, all items are used as the
 *   parameters of the operator. If this is Null, then the operator takes 0
 *   parameters. Otherwise this field is used as the only 1 parameter for the
 *   operator.
 *
 * The property must be filterable. Check field indices in schema/get (see
 * SchemaController::get() ). All index names (the @b name field in every
 * index) which @b filter is @c true can be used here.
 *
 * Example:
 *
 * @code
 * [
 *   {"property":"price", "operator":"range", "value":[1.0, 10.0]},
 *   {"property":"category", "operator":"in", "value":["sports", "music"]},
 *   {"property":"rating", "operator":">", "value":3}
 * ]
 * @endcode
 */

ConditionArrayParser::ConditionArrayParser()
{}

bool ConditionArrayParser::parse(const Value& conditions)
{
    clearMessages();
    parsers_.clear();

    if (conditions.type() == Value::kNullType)
    {
        return true;
    }

    const Value::ArrayType* array = conditions.getPtr<Value::ArrayType>();
    if (!array)
    {
        error() = "Conditions must be an array";
        return false;
    }

    if (array->size() == 0)
    {
        return true;
    }

    parsers_.push_back(ConditionParser());

    for (std::size_t i = 0; i < array->size(); ++i)
    {
        if (parsers_.back().parse((*array)[i]))
        {
            parsers_.push_back(ConditionParser());
        }
        else
        {
            if (!error().empty())
            {
                error() += "\n";
            }
            error() += parsers_.back().errorMessage();
        }
    }
    parsers_.pop_back();

    return error().empty();
}

}
