#ifndef PROCESS_CONTROLLERS_LOGANALYSIS_CONTROLLER_H
#define PROCESS_CONTROLLERS_LOGANALYSIS_CONTROLLER_H
/**
 * @file process/controllers/LogAnalysisController.h
 * @author Wei Cao
 * @date Created <2010-07-16>
 */
#include "Sf1Controller.h"
#include <common/Keys.h>
#include <util/driver/value/types.h>

#include <boost/lexical_cast.hpp>

#include <string>
#include <sstream>

namespace sf1r
{

using driver::Keys;
using namespace ::izenelib::driver;

/// @addtogroup controllers
/// @{

/**
 * @brief Controller \b LogAnalysis
 */
class LogAnalysisController : public ::izenelib::driver::Controller
{
public:
    void system_events();

    void user_queries();

    void merchant_count();

    void product_count();

    void product_update_info();

    void inject_user_queries();

    void delete_record_from_system_events();

    void delete_record_from_user_queries();

    void delete_database();

    void get_freq_user_queries_from_logserver();

    void inject_user_queries_to_logserver();
private:

    std::string parseSelect(bool & existAggregateFunc);

    std::string parseOrder();

    std::string parseConditions();

    bool parseConditions(std::string & collection, std::string & begin_time, std::string & end_time);

    std::string parseGroupBy();

    static std::string str_join( const std::vector<std::string> & list , const std::string & separator)
    {
        if (list.size() == 0) return "";

        std::stringstream writer;
        for ( size_t i =0; i<list.size()-1; i++ )
            writer << list[i] << separator;
        writer << list[list.size()-1];
        return writer.str();
    }

    template<typename T1, typename T2>
    static std::string str_concat( const T1 & arg1, const T2 & arg2 )
    {
        std::stringstream writer;
        writer << arg1 << arg2;
        return writer.str();
    }

    template<typename T1, typename T2, typename T3>
    static std::string str_concat( const T1 & arg1, const T2 & arg2, const T3 & arg3 )
    {
        return str_concat(str_concat(arg1, arg2), arg3);
    }

    template<typename T1, typename T2, typename T3, typename T4>
    static std::string str_concat( const T1 & arg1, const T2 & arg2, const T3 & arg3, const T4 & arg4 )
    {
        return str_concat(str_concat(arg1, arg2, arg3), arg4);
    }

    static std::string to_str( const Value & value )
    {
        switch ( value.type() )
        {
        case Value::kNullType:
            return "@NULL";
        case Value::kIntType:
            return boost::lexical_cast<std::string>(asInt(value));
        case Value::kUintType:
            return boost::lexical_cast<std::string>(asUint(value));
        case Value::kDoubleType:
            return boost::lexical_cast<std::string>(asDouble(value));
        case Value::kStringType:
            return str_concat("'",asString(value), "'");
        case Value::kBoolType:
            return boost::lexical_cast<std::string>(asBool(value));
        case Value::kArrayType:
            return "@Array";
        case Value::kObjectType:
            return "@Object";
        default:
            return "@Unknown";
        }
    }
};

/// @}

} // namespace sf1r

#endif // PROCESS_CONTROLLERS_LOGANALYSIS_CONTROLLER_H
