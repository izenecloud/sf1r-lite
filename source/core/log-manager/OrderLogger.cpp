#include "OrderLogger.h"
#include "DbConnection.h"

#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <glog/logging.h>

namespace sf1r
{

const char* OrderLogger::ColumnName[EoC] = {
    "Id",
    "StrId",
    "Collection",
    "UserId",
    "TimeStamp"
};

const char* OrderLogger::ColumnMeta[EoC] = {
    "INTEGER PRIMARY KEY",
    "TEXT",
    "TEXT",
    "TEXT",
    "TEXT"
};

const char* OrderLogger::TableName = "order_logs";

void OrderLogger::createTable()
{
    DbConnection& dbConnection = DbConnection::instance();
    std::ostringstream oss;

    oss << "create table " << TableName << "(";

    // as insertOrder() needs a unique id for each new row,
    // declare column "Id" with auto increment attribute
    oss << ColumnName[0] << " "
        << ColumnMeta[0] << " "
        << dbConnection.getSqlKeyword(DbConnection::ATTR_AUTO_INCREMENT) << ", ";

    for (int i=1; i<EoC; ++i)
    {
        oss << ColumnName[i] << " " << ColumnMeta[i];
        if (i != EoC-1)
        {
            oss << ", ";
        }
    }
    oss << ");";

    dbConnection.exec(oss.str(), true);
}

bool OrderLogger::insertOrder(
    const std::string& orderId,
    const std::string& collection,
    const std::string& userId,
    const std::string& date,
    int& newId
)
{
    if (collection.empty() || userId.empty())
    {
        LOG(ERROR) << "order log needs both the collection name and user id";
        return false;
    }

    std::string sql;
    prepareSql_(orderId, collection, userId, date, sql);

    RowList sqlResult;
    if (!DbConnection::instance().exec(sql, sqlResult))
        return false;

    if (!getLastIdFromRows_(sqlResult, newId))
        return false;

    return true;
}

void OrderLogger::prepareSql_(
    const std::string& orderId,
    const std::string& collection,
    const std::string& userId,
    const std::string& date,
    std::string& sql
) const
{
    boost::posix_time::ptime pt(boost::posix_time::second_clock::local_time());
    if (!date.empty())
    {
        try
        {
            boost::gregorian::date greDate = boost::gregorian::from_simple_string(date);
            pt = boost::posix_time::ptime(greDate);
        }
        catch(const std::exception& e)
        {
            LOG(WARNING) << "exception in converting date " << date
                         << ": " << e.what()
                         << ", expect format: YYYY-mm-dd, use current time instead";
        }
    }
    std::string isoTime = boost::posix_time::to_iso_string(pt);

    std::ostringstream oss;
    oss << "insert into " << TableName << " values("
        << "NULL, "
        << "\"" << orderId << "\", "
        << "\"" << collection << "\", "
        << "\"" << userId << "\", "
        << "\"" << isoTime << "\");";

    oss << "select " << getLastIdFunc_() << ";";
    sql = oss.str();
}

const std::string& OrderLogger::getLastIdFunc_() const
{
    DbConnection& dbConnection = DbConnection::instance();
    return dbConnection.getSqlKeyword(DbConnection::FUNC_LAST_INSERT_ID);
}

bool OrderLogger::getLastIdFromRows_(
    const RowList& selectRows,
    int& lastId
) const
{
    if (selectRows.size() != 1)
    {
        LOG(ERROR) << "expect one row, while current row number: " << selectRows.size();
        return false;
    }

    const Row& row = selectRows.front();
    const std::string& lastIdFunc = getLastIdFunc_();
    Row::const_iterator mapIt = row.find(lastIdFunc);
    if (mapIt == row.end())
    {
        LOG(ERROR) << "column " << lastIdFunc << " is not found in select row";
        return false;
    }

    const std::string& lastIdStr = mapIt->second;
    try
    {
        lastId = boost::lexical_cast<int>(lastIdStr);
    }
    catch(const boost::bad_lexical_cast& e)
    {
        LOG(ERROR) << "failed to convert last insert id: " << lastIdStr
                    << ", exception: " << e.what();
        return false;
    }

    return true;
}

}
