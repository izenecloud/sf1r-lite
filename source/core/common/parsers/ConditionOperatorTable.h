#ifndef CORE_DRIVER_PARSERS_CONDITION_OPERATOR_TABLE_H
#define CORE_DRIVER_PARSERS_CONDITION_OPERATOR_TABLE_H
/**
 * @file core/common/parsers/ConditionOperatorTable.h
 * @author Ian Yang
 * @date Created <2010-06-13 14:44:11>
 */
#include <boost/thread/once.hpp>
#include <boost/unordered_map.hpp>

namespace sf1r {

class ConditionOperatorTable
{
public:
    struct OperatorInfo
    {
        std::string name;
        std::size_t minArgCount;
        std::size_t maxArgCount;
    };
    typedef boost::unordered_map<std::string, OperatorInfo> table_type;

    static const table_type& get()
    {
        boost::call_once(init, flag_);
        return table_;
    }

    static void init();

private:
    static table_type table_;

    static boost::once_flag flag_;
};

}

#endif // CORE_DRIVER_PARSERS_CONDITION_OPERATOR_TABLE_H
