///
/// @file t_OrderLogger.cpp
/// @brief test OrderLogger to log orders
/// @author Jun Jiang
/// @date Created 2011-08-30
///

#include <log-manager/OrderLogger.h>
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
const string TEST_DIR_STR = "test_log_manager_dir/t_OrderLogger";
const string SQLITE_ORDER_DB_FILE = "order.db";
const string MYSQL_ORDER_DB_NAME = "order_logger_test_db";
}

struct OrderLoggerColumn
{
    const string id_;
    const string strId_;
    const string collection_;
    const string userId_;
    const string timeStamp_;

    OrderLoggerColumn()
        : id_(OrderLogger::ColumnName[OrderLogger::Id])
        , strId_(OrderLogger::ColumnName[OrderLogger::StrId])
        , collection_(OrderLogger::ColumnName[OrderLogger::Collection])
        , userId_(OrderLogger::ColumnName[OrderLogger::UserId])
        , timeStamp_(OrderLogger::ColumnName[OrderLogger::TimeStamp])
    {
    }
};

struct OrderLoggerTestFixture
{
    string dbPath_;
    string mySqlDBName_;

    RDbConnection& rdbConnection_;
    OrderLogger& orderLogger_;

    OrderLoggerColumn columns_;
    int orderId_;

    typedef map<string, string> Row;
    typedef list<Row> RowList;
    RowList rowList_;

    OrderLoggerTestFixture()
        : rdbConnection_(RDbConnection::instance())
        , orderLogger_(OrderLogger::instance())
        , orderId_(0)
    {
    }

    ~OrderLoggerTestFixture()
    {
        // remove test db
        if (dbPath_.find("sqlite3") == 0)
        {
            bfs::remove_all(TEST_DIR_STR);
        }
        else if (dbPath_.find("mysql") == 0)
        {
            stringstream sql;
            sql << "drop database " << mySqlDBName_ << ";";
            BOOST_CHECK(rdbConnection_.exec(sql.str()));
        }

        rdbConnection_.close();
    }

    void initSqlite()
    {
        bfs::remove_all(TEST_DIR_STR);
        bfs::create_directories(TEST_DIR_STR);
        dbPath_ = "sqlite3://" + (bfs::path(TEST_DIR_STR) / SQLITE_ORDER_DB_FILE).string();

        initDbConnection();
    }

    void initMysql()
    {
        posix_time::ptime pt(posix_time::second_clock::local_time());
        mySqlDBName_ = MYSQL_ORDER_DB_NAME + "_" + posix_time::to_iso_string(pt);
        dbPath_ = "mysql://root:123456@127.0.0.1:3306/" + mySqlDBName_;

        initDbConnection();
    }

    void initDbConnection()
    {
        cout << "dbPath: " << dbPath_ << endl;
        BOOST_CHECK(rdbConnection_.init(dbPath_));

        OrderLogger::createTable();
    }

    void resetDbConnection()
    {
        rdbConnection_.close();
        BOOST_CHECK(rdbConnection_.init(dbPath_));
    }

    void run()
    {
        // empty order
        checkOrders();

        // valid order, valid date
        insertOrder("order_001", "collection_001", "user_001", "2011-07-30");
        insertOrder("order_002", "collection_001", "user_002", "2011-08-30 11:01:05");
        insertOrder("order_003", "collection_002", "user_002", "2011-9-30");
        insertOrder("", "collection_001", "user_001", "2011-08-30");
        // valid order, invalid date
        insertOrder("order_003", "collection_002", "user_002", "2011-13-30");
        insertOrder("order_003", "collection_002", "user_002", "2011-09-31");
        insertOrder("order_001", "collection_001", "user_001", "20110830");
        insertOrder("order_001", "collection_001", "user_001", "ABCDE");
        insertOrder("order_001", "collection_001", "user_001", "");
        // invalid order, empty collection or user
        insertOrder("order_001", "", "user_001", "2011-08-30");
        insertOrder("order_001", "collection_001", "", "2011-08-30");
        insertOrder("order_001", "", "", "2011-08-30");

        checkOrders();

        resetDbConnection();
        checkOrders();

        // new order
        insertOrder("order_004", "collection_001", "user_004", "2011-10-30");
        insertOrder("", "collection_002", "user_005", "2011-11-30");
        checkOrders();
    }

    void checkOrders()
    {
        stringstream sql;
        sql << "select * from " << OrderLogger::TableName
            << " order by " << columns_.id_ << " asc;";

        RowList sqlResult;
        BOOST_CHECK(rdbConnection_.exec(sql.str(), sqlResult));

        BOOST_TEST_MESSAGE(print(sqlResult));
        BOOST_CHECK_EQUAL(sqlResult.size(), rowList_.size());

        RowList::iterator resultIt = sqlResult.begin();
        RowList::iterator listIt = rowList_.begin();
        for (; resultIt != sqlResult.end() && listIt != rowList_.end(); ++resultIt, ++listIt)
        {
            BOOST_CHECK_EQUAL((*resultIt)[columns_.id_], (*listIt)[columns_.id_]);
            BOOST_CHECK_EQUAL((*resultIt)[columns_.strId_], (*listIt)[columns_.strId_]);
            BOOST_CHECK_EQUAL((*resultIt)[columns_.collection_], (*listIt)[columns_.collection_]);
            BOOST_CHECK_EQUAL((*resultIt)[columns_.userId_], (*listIt)[columns_.userId_]);

            const string& timeStamp = (*resultIt)[columns_.timeStamp_];
            try
            {
                posix_time::ptime pt(posix_time::from_iso_string(timeStamp));
                BOOST_CHECK_MESSAGE(pt.is_not_a_date_time() == false, "TimeStamp is not a date time: " + timeStamp);
            }
            catch(const std::exception& e)
            {
                BOOST_CHECK_MESSAGE(false, "invalid TimeStamp format: " + timeStamp);
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

    void insertOrder(
        const string& order,
        const string& collection,
        const string& user,
        const string& date
    )
    {
        int newId = 0;
        bool result = orderLogger_.insertOrder(order, collection, user, date, newId);

        if (collection.empty() || user.empty())
        {
            BOOST_CHECK(result == false);
        }
        else
        {
            BOOST_CHECK(result);
            BOOST_CHECK_EQUAL(newId, ++orderId_);

            Row row;
            row[columns_.id_] = lexical_cast<string>(orderId_);
            row[columns_.strId_] = order;
            row[columns_.collection_] = collection;
            row[columns_.userId_] = user;

            rowList_.push_back(row);
        }
    }
};

BOOST_AUTO_TEST_SUITE(OrderLoggerTest)

BOOST_FIXTURE_TEST_CASE(checkSqliteInsertOrder, OrderLoggerTestFixture)
{
    initSqlite();
    run();
}

BOOST_FIXTURE_TEST_CASE(checkMysqlInsertOrder, OrderLoggerTestFixture)
{
    initMysql();
    run();
}

BOOST_AUTO_TEST_SUITE_END() 
