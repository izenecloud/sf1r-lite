#include "DateStrFormat.h"

#include <boost/format.hpp>

NS_FACETED_BEGIN

DateStrFormat::DateStrFormat()
    : dateValueParsers_(DATE_INDEX_NUM)
    , dateRanges_(DATE_INDEX_NUM)
    , apiDateStrFormats_(DATE_INDEX_NUM)
    , scdDateOffsets_(DATE_INDEX_NUM)
    , SCD_DATE_STR_SIZE(std::string("YYYYMMDDhhmmss").size())
{
    // MASK_YEAR = 0x7FFE0000
    dateValueParsers_[DATE_INDEX_YEAR] = DateValueParser(MASK_YEAR, 17);
    // MASK_MONTH = 0x1E000
    dateValueParsers_[DATE_INDEX_MONTH] = DateValueParser(MASK_MONTH, 13);
    // MASK_DAY = 0x1F00
    dateValueParsers_[DATE_INDEX_DAY] = DateValueParser(MASK_DAY, 8);

    dateRanges_[DATE_INDEX_YEAR] = DateRange(0, 9999);
    dateRanges_[DATE_INDEX_MONTH] = DateRange(1, 12);
    dateRanges_[DATE_INDEX_DAY] = DateRange(1, 31);

    // date format "YYYY", "YYYY-MM", "YYYY-MM-DD" in API
    apiDateStrFormats_[DATE_INDEX_YEAR] = "%|04|";
    apiDateStrFormats_[DATE_INDEX_MONTH] = "%|04|-%|02|";
    apiDateStrFormats_[DATE_INDEX_DAY] = "%|04|-%|02|-%|02|";

    // separate tokens from API date such as "YYYY", "YYYY-MM", "YYYY-MM-DD"
    apiDateSepFunc_ = boost::char_separator<char>(
        "-", "", boost::keep_empty_tokens);

    // date format "YYYYMMDD" in SCD
    scdDateOffsets_[DATE_INDEX_YEAR] = 4;
    scdDateOffsets_[DATE_INDEX_MONTH] = 2;
    scdDateOffsets_[DATE_INDEX_DAY] = 2;

    // the "false" in below constructor makes tokenizer stop when
    // all the offsets have been used.
    scdDateSepFunc_ = boost::offset_separator(
        scdDateOffsets_.begin(),
        scdDateOffsets_.end(),
        false);
}

bool DateStrFormat::apiDateStrToDateVec(
    const std::string& dateStr,
    DateVec& dateVec
)
{
    APIDateTokenizer tok(dateStr, apiDateSepFunc_);
    return tokenizeDate_(tok.begin(), tok.end(), dateVec);
}

bool DateStrFormat::scdDateStrToDateVec(
    const std::string& dateStr,
    DateVec& dateVec
)
{
    if (dateStr.size() != SCD_DATE_STR_SIZE)
        return false;

    ScdDateTokenizer tok(dateStr, scdDateSepFunc_);
    return tokenizeDate_(tok.begin(), tok.end(), dateVec);
}

DateGroupTable::date_t DateStrFormat::createDate(const DateVec& dateVec)
{
    DateGroupTable::date_t date = 0;
    const std::size_t tokNum = dateVec.size();
    assert(tokNum > 0 && tokNum <= DATE_INDEX_NUM);

    for (std::size_t i = 0; i < tokNum; ++i)
    {
        date |= dateValueParsers_[i].create(dateVec[i]);
    }

    return date;
}

void DateStrFormat::extractDate(DateGroupTable::date_t date, DateVec& dateVec)
{
    dateVec.resize(DATE_INDEX_NUM);

    for (std::size_t i = 0; i < DATE_INDEX_NUM; ++i)
    {
        dateVec[i] = dateValueParsers_[i].extract(date);
    }
}

void DateStrFormat::formatAPIDateStr(const DateVec& dateVec, std::string& str)
{
    std::size_t lastIndex = 0;
    for (; lastIndex < DATE_INDEX_NUM; ++lastIndex)
    {
        if (dateVec[lastIndex] == 0)
            break;
    }

    if (lastIndex)
    {
        --lastIndex;
    }

    boost::format fmt(apiDateStrFormats_[lastIndex]);
    for (std::size_t i = 0; i <= lastIndex; ++i)
    {
        fmt % dateVec[i];
    }

    str = fmt.str();
}

bool DateStrFormat::checkDateRange_(const DateVec& dateVec)
{
    const std::size_t tokNum = dateVec.size();
    if (tokNum < 1 || tokNum > DATE_INDEX_NUM)
        return false;

    for (std::size_t i = 0; i < tokNum; ++i)
    {
        DateGroupTable::date_t date = dateVec[i];
        const DateRange& range = dateRanges_[i];

        if (date < range.first || date > range.second)
            return false;
    }

    return true;
}

NS_FACETED_END
