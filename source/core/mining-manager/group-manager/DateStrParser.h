///
/// @file DateStrParser.h
/// @brief parsing between date and string
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-07-05
///

#ifndef SF1R_DATE_STR_PARSER_H_
#define SF1R_DATE_STR_PARSER_H_

#include "../faceted-submanager/faceted_types.h"
#include "DateGroupTable.h"
#include "DateStrFormat.h"
#include <util/singleton.h>

#include <string>
#include <map>
#include <vector>

NS_FACETED_BEGIN

class DateStrParser
{
public:
    static DateStrParser* get()
    {
        return izenelib::util::Singleton<DateStrParser>::get();
    }

    DateStrParser();

    struct DateMask
    {
        DateGroupTable::date_t date_;
        DATE_MASK_TYPE mask_;

        DateMask() : date_(0), mask_(MASK_YEAR_MONTH_DAY) {}
    };

    /**
     * Given @p unitStr of "Y", "M", "D",
     * it assigns @p maskType with mask type of year, month, day respectively,
     * and returns true; for invalid @p unitStr, it returns false.
     */
    bool unitStrToMask(
        const std::string& unitStr,
        DATE_MASK_TYPE& maskType,
        std::string& errorMsg
    );

    /**
     * Given @p dateStr such as "2012", "2012-07", "2012-07-04",
     * it assigns @p dateMask with @c DateMask of year, month, day respectively,
     * and returns true; for invalid @p dateStr, it returns false.
     */
    bool apiStrToDateMask(
        const std::string& dateStr,
        DateMask& dateMask,
        std::string& errorMsg
    );

    /**
     * Given @p scdStr format such as "20120704143015",
     * it assigns @p date with [year:2012, month:07, day:04], and returns true;
     * for invalid @p scdStr, it returns false.
     */
    bool scdStrToDate(
        const std::string& scdStr,
        DateGroupTable::date_t& date,
        std::string& errorMsg
    );

    /**
     * Given @p date value of year, month or day, it assigns @p str such as
     * "2012", "2012-07", "2012-07-04".
     */
    void dateToAPIStr(
        DateGroupTable::date_t date,
        std::string& str
    );

private:
    DateStrFormat format_;

    /** unit str => mask value */
    typedef std::map<std::string, DATE_MASK_TYPE> UnitMaskMap;
    UnitMaskMap unitMaskMap_;

    /** mask values for year, year-month, year-month-day */
    std::vector<DATE_MASK_TYPE> dateMasks_;
};

NS_FACETED_END

#endif // SF1R_DATE_STR_PARSER_H_
