///
/// @file t_ItemLogger.cpp
/// @brief test ItemLogger to log items
/// @author Jun Jiang
/// @date Created 2011-08-31
///

#include <log-manager/ItemLogger.h>
#include <log-manager/DbConnection.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/lexical_cast.hpp>

#include <string>
#include <list>
#include <map>
#include <iostream>
#include <sstream>

using namespace std;
using namespace boost;
using namespace sf1r;

namespace bfs = boost::filesystem;

namespace
{
const char* TEST_DIR_STR = "test_log_manager_dir/t_ItemLogger";
const char* ITEM_DB_STR = "item.db";
}

struct ItemLoggerColumn
{
    const string orderId_;
    const string itemId_;
    const string price_;
    const string quantity_;
    const string isRec_;

    ItemLoggerColumn()
        : orderId_(ItemLogger::ColumnName[ItemLogger::OrderId])
        , itemId_(ItemLogger::ColumnName[ItemLogger::ItemId])
        , price_(ItemLogger::ColumnName[ItemLogger::Price])
        , quantity_(ItemLogger::ColumnName[ItemLogger::Quantity])
        , isRec_(ItemLogger::ColumnName[ItemLogger::IsRec])
    {
    }
};

struct ItemRecord
{
    int orderId_;
    string itemId_;
    double price_;
    int quantity_;
    bool isRec_;

    ItemRecord(int orderId, string itemId, double price, int quantity, bool isRec)
        : orderId_(orderId)
        , itemId_(itemId)
        , price_(price)
        , quantity_(quantity)
        , isRec_(isRec)
    {}
};

struct ItemLoggerTestFixture
{
    const string dbPath_;
    DbConnection& dbConnection_;
    ItemLogger& itemLogger_;
    ItemLoggerColumn columns_;
    int rowNum_;

    typedef map<string, string> Row;
    typedef list<Row> RowList;

    typedef list<ItemRecord> RecordList;
    RecordList recordList_;

    ItemLoggerTestFixture()
        : dbPath_((bfs::path(TEST_DIR_STR) / ITEM_DB_STR).string())
        , dbConnection_(DbConnection::instance())
        , itemLogger_(ItemLogger::instance())
        , rowNum_(0)
    {
        bfs::remove_all(TEST_DIR_STR);
        bfs::create_directories(TEST_DIR_STR);

        BOOST_CHECK(dbConnection_.init(dbPath_));
        ItemLogger::createTable();
    }

    ~ItemLoggerTestFixture()
    {
        dbConnection_.close();
    }

    void resetDbConnection()
    {
        dbConnection_.close();
        BOOST_CHECK(dbConnection_.init(dbPath_));
    }

    void insertItem(
        int orderId,
        const string& itemId,
        double price,
        int quantity,
        bool isRec
    )
    {
        bool result = itemLogger_.insertItem(orderId, itemId, price, quantity, isRec);

        if (orderId <= 0 || itemId.empty())
        {
            BOOST_CHECK(result == false);
        }
        else
        {
            BOOST_CHECK(result);

            ++rowNum_;
            recordList_.push_back(ItemRecord(orderId, itemId, price, quantity, isRec));
        }
    }

    void checkItems()
    {
        stringstream sql;
        sql << "select * from " << ItemLogger::TableName << ";";

        RowList sqlResult;
        BOOST_CHECK(dbConnection_.exec(sql.str(), sqlResult));

        BOOST_TEST_MESSAGE(print(sqlResult));
        BOOST_CHECK_EQUAL(sqlResult.size(), recordList_.size());

        RowList::iterator resultIt = sqlResult.begin();
        RecordList::iterator recordIt = recordList_.begin();
        for (; resultIt != sqlResult.end(); ++resultIt, ++recordIt)
        {
            const string& orderIdStr = (*resultIt)[columns_.orderId_];
            int orderId = lexical_cast<int>(orderIdStr);
            BOOST_CHECK_EQUAL(orderId, recordIt->orderId_);

            const string& itemIdStr = (*resultIt)[columns_.itemId_];
            BOOST_CHECK_EQUAL(itemIdStr, recordIt->itemId_);

            const string& priceStr = (*resultIt)[columns_.price_];
            double price = lexical_cast<double>(priceStr);
            BOOST_CHECK_EQUAL(price, recordIt->price_);

            const string& quantityStr = (*resultIt)[columns_.quantity_];
            int quantity = lexical_cast<int>(quantityStr);
            BOOST_CHECK_EQUAL(quantity, recordIt->quantity_);

            const string& isRecStr = (*resultIt)[columns_.isRec_];
            try
            {
                bool isRec = lexical_cast<bool>(isRecStr);
                BOOST_CHECK_EQUAL(isRec, recordIt->isRec_);
            }
            catch(const std::exception& e)
            {
                BOOST_CHECK_MESSAGE(false, "invalid isRec string: " + isRecStr);
            }
        }
    }

    string print(const RowList& rowList)
    {
        ostringstream oss;
        oss << "print row num: " << rowList.size() << endl;
        for (RowList::const_iterator listIt = rowList.begin();
            listIt != rowList.end(); ++listIt)
        {
            for (Row::const_iterator mapIt = listIt->begin();
                mapIt != listIt->end(); ++mapIt)
            {
                oss << "(" << mapIt->first << ": " << mapIt->second << "), ";
            }
            oss << endl;
        }
        return oss.str();
    }
};

BOOST_AUTO_TEST_SUITE(ItemLoggerTest)

BOOST_FIXTURE_TEST_CASE(checkInsertItem, ItemLoggerTestFixture)
{
    // empty item
    checkItems();

    // valid item
    insertItem(1, "item_001", 10.0, 1, false);
    insertItem(2, "item_002", 2.5, 4, true);
    insertItem(2, "item_002", 1338.210, 2, false);
    insertItem(3, "item_003", 0.0, 5, true);
    insertItem(3, "item_003", -12.35, 3, true);
    // invalid orderId or itemId
    insertItem(0, "item_005", 10.0, 1, false);
    insertItem(-2, "item_006", 10.0, 1, false);
    insertItem(1, "", 10.0, 1, false);

    checkItems();

    resetDbConnection();
    checkItems();

    // new item
    insertItem(4, "item_004", 100.0, 1, false);
    insertItem(5, "item_005", 1000.0, 2, true);
    checkItems();
}

BOOST_AUTO_TEST_SUITE_END() 
