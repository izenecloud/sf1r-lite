///
/// @file t_ItemLogger.cpp
/// @brief test ItemLogger to log items
/// @author Jun Jiang
/// @date Created 2011-08-31
///

#include <log-manager/ItemLogger.h>
#include <log-manager/RDbConnection.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

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
const string TEST_DIR_STR = "test_log_manager_dir/t_ItemLogger";
const string SQLITE_ITEM_DB_FILE = "item.db";
const string MYSQL_ITEM_DB_NAME = "item_logger_test_db";
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
    string dbPath_;
    string mySqlDBName_;

    RDbConnection& rdbConnection_;
    ItemLogger& itemLogger_;

    ItemLoggerColumn columns_;
    int rowNum_;
    bool isConnect_;

    typedef map<string, string> Row;
    typedef list<Row> RowList;

    typedef list<ItemRecord> RecordList;
    RecordList recordList_;

    ItemLoggerTestFixture()
        : rdbConnection_(RDbConnection::instance())
        , itemLogger_(ItemLogger::instance())
        , rowNum_(0)
        , isConnect_(false)
    {
    }

    ~ItemLoggerTestFixture()
    {
        // remove test db
        if (dbPath_.find("sqlite3") == 0)
        {
            bfs::remove_all(TEST_DIR_STR);
        }
        else if (dbPath_.find("mysql") == 0 && isConnect_)
        {
            stringstream sql;
            sql << "drop database " << mySqlDBName_ << ";";
            BOOST_CHECK(rdbConnection_.exec(sql.str()));
        }

        rdbConnection_.close();
    }

    bool initSqlite()
    {
        bfs::remove_all(TEST_DIR_STR);
        bfs::create_directories(TEST_DIR_STR);
        dbPath_ = "sqlite3://" + (bfs::path(TEST_DIR_STR) / SQLITE_ITEM_DB_FILE).string();

        return initDbConnection();
    }

    bool initMysql()
    {
        posix_time::ptime pt(posix_time::second_clock::local_time());
        mySqlDBName_ = MYSQL_ITEM_DB_NAME + "_" + posix_time::to_iso_string(pt);
        dbPath_ = "mysql://root:123456@127.0.0.1:3306/" + mySqlDBName_;

        return initDbConnection();
    }

    bool initDbConnection()
    {
        cout << "dbPath: " << dbPath_ << endl;
        isConnect_ = rdbConnection_.init(dbPath_);

        if (isConnect_)
        {
            ItemLogger::createTable();
        }

        return isConnect_;
    }

    void resetDbConnection()
    {
        rdbConnection_.close();
        BOOST_CHECK(rdbConnection_.init(dbPath_));
    }

    void run()
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

    void checkItems()
    {
        stringstream sql;
        sql << "select * from " << ItemLogger::TableName << ";";

        RowList sqlResult;
        BOOST_CHECK(rdbConnection_.exec(sql.str(), sqlResult));

        BOOST_TEST_MESSAGE(print(sqlResult));
        BOOST_CHECK_EQUAL(sqlResult.size(), recordList_.size());

        RowList::iterator resultIt = sqlResult.begin();
        RecordList::iterator recordIt = recordList_.begin();
        for (; resultIt != sqlResult.end() && recordIt != recordList_.end(); ++resultIt, ++recordIt)
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
};

BOOST_AUTO_TEST_SUITE(ItemLoggerTest)

BOOST_FIXTURE_TEST_CASE(checkSqliteInsertItem, ItemLoggerTestFixture)
{
    BOOST_CHECK(initSqlite());

    run();
}

BOOST_FIXTURE_TEST_CASE(checkMysqlInsertItem, ItemLoggerTestFixture)
{
    if (! initMysql())
    {
        cerr << "warning: exit test case as failed to connect " << dbPath_ << endl;
        return;
    }

    run();
}

BOOST_AUTO_TEST_SUITE_END() 
