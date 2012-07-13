///
/// @file DateStrFormat.h
/// @brief format between date and string
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-07-05
///

#ifndef SF1R_DATE_STR_FORMAT_H_
#define SF1R_DATE_STR_FORMAT_H_

#include "../faceted-submanager/faceted_types.h"
#include "DateGroupTable.h"

#include <string>
#include <vector>
#include <utility>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

NS_FACETED_BEGIN

class DateStrFormat
{
public:
    enum DATE_INDEX_TYPE
    {
        DATE_INDEX_YEAR = 0,
        DATE_INDEX_MONTH,
        DATE_INDEX_DAY,
        DATE_INDEX_NUM
    };

    typedef std::vector<DateGroupTable::date_t> DateVec;

    DateStrFormat();

    /**
     * Given @p dateStr such as "2012", "2012-07", "2012-07-04",
     * it extracts year, month, day, put into @p dateVec,
     * and returns true; for invalid @p dateStr, it returns false.
     */
    bool apiDateStrToDateVec(
        const std::string& dateStr,
        DateVec& dateVec
    );

    /**
     * Given @p dateStr such as "20120704143015",
     * it extracts year, month, day, put into @p dateVec,
     * and returns true; for invalid @p dateStr, it returns false.
     */
    bool scdDateStrToDateVec(
        const std::string& dateStr,
        DateVec& dateVec
    );

    /** create date value from year, month, day in @p dateVec */
    DateGroupTable::date_t createDate(const DateVec& dateVec);

    /** extract year, month, day from @p date value */
    void extractDate(DateGroupTable::date_t date, DateVec& dateVec);

    /** output API date string of year, month, day delimited by "-" */
    void formatAPIDateStr(const DateVec& dateVec, std::string& str);

private:
    /** tokenize iterator [begin, end) into year, month, day */
    template <typename InputIter>
    bool tokenizeDate_(InputIter begin, InputIter end, DateVec& dateVec);

    /** check whether year, month, day are in valid range */
    bool checkDateRange_(const DateVec& dateVec);

private:
    class DateValueParser;

    std::vector<DateValueParser> dateValueParsers_;

    /** range [start, end] */
    typedef std::pair<DateGroupTable::date_t, DateGroupTable::date_t> DateRange;
    std::vector<DateRange> dateRanges_;

    std::vector<std::string> apiDateStrFormats_;

    typedef boost::char_separator<char> APIDateSepFunc;
    typedef boost::tokenizer<APIDateSepFunc> APIDateTokenizer;
    APIDateSepFunc apiDateSepFunc_;

    std::vector<int> scdDateOffsets_;
    const std::size_t SCD_DATE_STR_SIZE;

    typedef boost::offset_separator SCDDateSepFunc;
    typedef boost::tokenizer<boost::offset_separator> ScdDateTokenizer;
    SCDDateSepFunc scdDateSepFunc_;
};

template <typename InputIter>
bool DateStrFormat::tokenizeDate_(InputIter begin, InputIter end, DateVec& dateVec)
{
    try
    {
        for (InputIter it = begin; it != end; ++it)
        {
            DateGroupTable::date_t date =
                boost::lexical_cast<DateGroupTable::date_t>(*it);

            dateVec.push_back(date);
        }
    }
    catch (const std::exception& e)
    {
        return false;
    }

    return checkDateRange_(dateVec);
}

/** parse value for year, month, day */
class DateStrFormat::DateValueParser
{
public:
    DateValueParser() : mask_(MASK_ZERO), shift_(0) {}

    DateValueParser(DATE_MASK_TYPE mask, int shift)
        : mask_(mask)
        , shift_(shift)
    {}

    DateGroupTable::date_t extract(DateGroupTable::date_t date)
    {
        return (date & mask_) >> shift_;
    }

    DateGroupTable::date_t create(DateGroupTable::date_t date)
    {
        return date << shift_;
    }

private:
    DATE_MASK_TYPE mask_;
    int shift_;
};

NS_FACETED_END

#endif // SF1R_DATE_STR_FORMAT_H_
