/**
 * @file CustomRankingParser.cpp
 * @author Zhongxia Li
 * @date Apr 18, 2011
 */
#include "CustomRankingParser.h"

#include <string>
#include <sstream>

#include <boost/algorithm/string.hpp>

#include <common/Keys.h>
#include <common/IndexBundleSchemaHelpers.h>
#include <query-manager/QueryManager.h>

namespace sf1r {

using ::sf1r::driver::Keys;

/**
 * @class CustomRankingParser
 *
 * Field \b custom_rank is an object, which specifies how to evaluate custom ranking score
 * based on user defined expression over sortable properties. <br />
 * Note that, if \b sort field is not set, "custom_rank" is set as a sort property in descending order by default. <br />
 * But if \b sort field is explicitly set, "custom_rank" should be set as a sort property manually to enable custom
 * ranking feature.
 *
 * - @b custom_rank (@c Object): Custom Ranking request information.
 *   The @b expression is required, @b params is optional.
 *   - @b expression (@c String): Arithmetic expression used to evaluate custom ranking score.
 *     An expression is consist of operators, parameters.
 *     - operators: +, -, *, /, log(x), sqrt(x), pow(x,y)
 *     - parameters: see @b params, constant value or property name also can be used directly in expression.
 *   - @b params (@c Array): parameters occurred in expression
 *     - @b name (@c String): parameter name, include alphabets, numbers or '_', start with an alphabet or '_'.
 *     - @b type (@c String): parameter type, "CONSTANT" or "PROPERTY".
 *     - @b value (@c String):
 *       - "CONSTANT": real number.
 *       - "PROPERTY": property name of a numeric data field (sortable property).
 *
 * Example
 * @code
 * {
 *   "custom_rank":
 *   {
 *     "expression":"p1 * pow (x, 2) + p2",
 *     "params":
 *     [
 *       {
 *         "name":"p1",
 *         "type":"CONSTANT",
 *         "value":"0.5"
 *       },
 *       {
 *         "name":"p2",
 *         "type":"CONSTANT",
 *         "value":"100"
 *       },
 *       {
 *         "name":"x",
 *         "type":"PROPERTY",
 *         "value":"Price"
 *       }
 *     ]
 *   }
 * }
 * @endcode
 */

bool CustomRankingParser::parse(const Value& custom_rank)
{
    if (custom_rank.type() == Value::kNullType)
    {
        return true;
    }

    // parse params firstly
    if (custom_rank.hasKey(Keys::params)) {
        const Value::ArrayType* array = custom_rank[Keys::params].getPtr<Value::ArrayType>();

        for (std::size_t i = 0; i < array->size(); i++) {
            if (!parse_param((*array)[i])) {
                return false;
            }
        }
    }

    // parse expression
    if (custom_rank.hasKey(Keys::expression)) {
        std::string expr = asString(custom_rank[Keys::expression]);
        if (!customRanker_->parse(expr)) {
            error() = customRanker_->getErrorInfo();
            return false;
        }
    }
    else {
        error() = "custom_rank[expression] field must be set.";
        return false;
    }

    // check properties occurred in expression, and get property data type.
    // properties should be existed and sortable.
    std::map<std::string, PropertyDataType>& propertyDataTypeMap = customRanker_->getPropertyDataTypeMap();

    std::map<std::string, PropertyDataType>::iterator iter;
    for (iter = propertyDataTypeMap.begin(); iter != propertyDataTypeMap.end(); iter++) {
        if (!isPropertySortable(indexSchema_, iter->first)) {
            error() = "Please ensure \"" + iter->first + "\" is a parameter set in custom_rank[params], or a numeric data property.";
            return false;
        }
        // set property data type
        iter->second = getPropertyDataType(indexSchema_, iter->first);
        //cout << "***** property: " << iter->first  << "  datatype: " << iter->second << endl;
    }

    return true;
}

bool CustomRankingParser::parse_param(const Value& param)
{
    std::string name, type, value;
    stringstream ss_err;

    if ((name = asString(param[Keys::name])).empty())
    {
        ss_err << "custom_rank[params][name] should not be null." << endl;
        error() = ss_err.str();
        return false;
    }

    if (!CustomRanker::validateName(name)) {
        error() = "Invalid name in custom_rank[params] \"" + name +
                "\", should be alphabets, numbers or '_', do not start with a number.";
        return false;
    }

    type = asString(param[Keys::type]);
    value = asString(param[Keys::value]);

    if (boost::iequals(type, "CONSTANT"))
    {
        double dbValue;
        if (!CustomRanker::str2real(value, dbValue)) {
            ss_err << "Failed to parse custom_rank[params][value] \""<<
                    asString(param[Keys::value]) << "\", as a real number." << endl;
            error() = ss_err.str();
            return false;
        }

        //cout << "[CONSTANT] "<< name << ": " << dbValue << " type: "<<Value::type2Name(param[Keys::value].type()) << endl;
        customRanker_->addConstantParam(name, dbValue);
    }
    else if (boost::iequals(type, "PROPERTY"))
    {
        //cout << "[PROPERTY] "<< name << ": " << value << endl;
        customRanker_->addPropertyParam(name, value);
    }
    else
    {
        ss_err << "custom_rank[params][type] should be CONSTANT or PROPERTY, but get \"" << type << "\"." << endl;
        error() = ss_err.str();
        return false;
    }

    return true;
}

}

