///
/// @file t_DateGroupTable.cpp
/// @brief test DateGroupTable to store date values for each doc
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-07-04
///

#include <mining-manager/group-manager/DateGroupTable.h>
#include <mining-manager/group-manager/DateStrParser.h>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include "../recommend-manager/test_util.h"

using namespace sf1r::faceted;
namespace bfs = boost::filesystem;

namespace
{

const std::string TEST_DIR = "date_group_table_test";
const std::string PROP_NAME = "Group_date";

DateStrParser& DATE_STR_PARSER = *DateStrParser::get();

void createDateSet1(DateGroupTable::DateSet& dateSet)
{
    DateGroupTable::date_t date = 0;
    std::string errorMsg;

    BOOST_CHECK(DATE_STR_PARSER.scdStrToDate("20120704180130", date, errorMsg));
    dateSet.insert(date);
}

void createDateSet2(DateGroupTable::DateSet& dateSet)
{
    DateGroupTable::date_t date = 0;
    std::string errorMsg;

    BOOST_CHECK(DATE_STR_PARSER.scdStrToDate("20121213141516", date, errorMsg));
    dateSet.insert(date);

    BOOST_CHECK(DATE_STR_PARSER.scdStrToDate("20121231102030", date, errorMsg));
    dateSet.insert(date);

    BOOST_CHECK(DATE_STR_PARSER.scdStrToDate("20130102030405", date, errorMsg));
    dateSet.insert(date);

    BOOST_CHECK(DATE_STR_PARSER.scdStrToDate("20130102131415", date, errorMsg));
    dateSet.insert(date);
}

void validDateSet(
    const DateGroupTable::DateSet& dateSet,
    const std::string& goldDateStr
)
{
    std::vector<std::string> goldDateStrVec;
    sf1r::split_str_to_items(goldDateStr, goldDateStrVec);

    std::vector<std::string> dateStrVec;
    std::string dateStrOut;

    for (DateGroupTable::DateSet::const_iterator it = dateSet.begin();
        it != dateSet.end(); ++it)
    {
        DATE_STR_PARSER.dateToAPIStr(*it, dateStrOut);
        dateStrVec.push_back(dateStrOut);
    }

    BOOST_CHECK_EQUAL_COLLECTIONS(dateStrVec.begin(), dateStrVec.end(),
                                  goldDateStrVec.begin(), goldDateStrVec.end());
}

void checkGetDateSet(const DateGroupTable& dateTable)
{
    DateGroupTable::DateSet dateSet;

    sf1r::docid_t docId = 1;
    dateTable.getDateSet(docId, MASK_YEAR, dateSet);
    validDateSet(dateSet, "2012");

    dateTable.getDateSet(docId, MASK_YEAR_MONTH, dateSet);
    validDateSet(dateSet, "2012-07");

    dateTable.getDateSet(docId, MASK_YEAR_MONTH_DAY, dateSet);
    validDateSet(dateSet, "2012-07-04");

    docId = 2;
    dateTable.getDateSet(docId, MASK_YEAR, dateSet);
    validDateSet(dateSet, "2012 2013");

    dateTable.getDateSet(docId, MASK_YEAR_MONTH, dateSet);
    validDateSet(dateSet, "2012-12 2013-01");

    dateTable.getDateSet(docId, MASK_YEAR_MONTH_DAY, dateSet);
    validDateSet(dateSet, "2012-12-13 2012-12-31 2013-01-02");

    docId = 0;
    dateTable.getDateSet(docId, MASK_YEAR_MONTH_DAY, dateSet);
    BOOST_CHECK(dateSet.empty());

    docId = 3;
    dateTable.getDateSet(docId, MASK_YEAR_MONTH, dateSet);
    BOOST_CHECK(dateSet.empty());
}

}

BOOST_AUTO_TEST_SUITE(DateGroupTableTest)

BOOST_AUTO_TEST_CASE(testGetDateSet)
{
    bfs::remove_all(TEST_DIR);
    bfs::create_directory(TEST_DIR);

    {
        DateGroupTable dateTable(TEST_DIR, PROP_NAME);
        BOOST_CHECK(dateTable.open());

        const std::string& propName = dateTable.propName();
        BOOST_CHECK_EQUAL(propName, PROP_NAME);
        BOOST_CHECK_EQUAL(dateTable.docIdNum(), 1U);

        DateGroupTable::DateSet dateSet1;
        createDateSet1(dateSet1);
        dateTable.appendDateSet(dateSet1);

        DateGroupTable::DateSet dateSet2;
        createDateSet2(dateSet2);
        dateTable.appendDateSet(dateSet2);

        BOOST_CHECK_EQUAL(dateTable.docIdNum(), 3U);

        checkGetDateSet(dateTable);

        BOOST_CHECK(dateTable.flush());
    }

    {
        DateGroupTable dateTable(TEST_DIR, PROP_NAME);
        BOOST_CHECK(dateTable.open());

        BOOST_CHECK_EQUAL(dateTable.docIdNum(), 3U);

        checkGetDateSet(dateTable);
    }
}

BOOST_AUTO_TEST_SUITE_END() 
