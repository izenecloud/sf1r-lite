#include "DateStrParser.h"

NS_FACETED_BEGIN

DateStrParser::DateStrParser()
    : dateMasks_(DateStrFormat::DATE_INDEX_NUM)
{
    unitMaskMap_["Y"] = MASK_YEAR;
    unitMaskMap_["M"] = MASK_YEAR_MONTH;
    unitMaskMap_["D"] = MASK_YEAR_MONTH_DAY;

    dateMasks_[DateStrFormat::DATE_INDEX_YEAR] = MASK_YEAR;
    dateMasks_[DateStrFormat::DATE_INDEX_MONTH] = MASK_YEAR_MONTH;
    dateMasks_[DateStrFormat::DATE_INDEX_DAY] = MASK_YEAR_MONTH_DAY;
}

bool DateStrParser::unitStrToMask(
    const std::string& unitStr,
    DATE_MASK_TYPE& maskType,
    std::string& errorMsg
)
{
    UnitMaskMap::const_iterator it = unitMaskMap_.find(unitStr);

    if (it != unitMaskMap_.end())
    {
        maskType = it->second;
        return true;
    }

    errorMsg = "invalid parameter of [group][unit]: " + unitStr;
    return false;
}

bool DateStrParser::apiStrToDateMask(
    const std::string& dateStr,
    DateMask& dateMask,
    std::string& errorMsg
)
{
    DateStrFormat::DateVec dateVec;

    if (! format_.apiDateStrToDateVec(dateStr, dateVec))
    {
        errorMsg = "invalid date format of [search][group_label][value]: " + dateStr;
        return false;
    }

    dateMask.date_ = format_.createDate(dateVec);
    dateMask.mask_ = dateMasks_[dateVec.size()-1];

    return true;
}

bool DateStrParser::scdStrToDate(
    const std::string& scdStr,
    DateGroupTable::date_t& date,
    std::string& errorMsg
)
{
    DateStrFormat::DateVec dateVec;

    if (!format_.scdDateStrToDateVec(scdStr, dateVec) ||
        dateVec.size() != DateStrFormat::DATE_INDEX_NUM)
    {
        errorMsg = "invalid date format: " + scdStr;
        return false;
    }

    date = format_.createDate(dateVec);
    return true;
}

void DateStrParser::dateToAPIStr(
    DateGroupTable::date_t date,
    std::string& str
)
{
    DateStrFormat::DateVec dateVec;

    format_.extractDate(date, dateVec);

    format_.formatAPIDateStr(dateVec, str);
}

NS_FACETED_END
