#include "ItemLogger.h"
#include "RDbConnection.h"

#include <sstream>
#include <glog/logging.h>

namespace sf1r
{

const char* ItemLogger::ColumnName[EoC] =
{
    "OrderId",
    "ItemId",
    "Price",
    "Quantity",
    "IsRec"
};

const char* ItemLogger::ColumnMeta[EoC] =
{
    "INTEGER",
    "TEXT",
    "REAL",
    "INTEGER",
    "INTEGER"
};

const char* ItemLogger::TableName = "item_logs";

void ItemLogger::createTable()
{
    RDbConnection& rdbConnection = RDbConnection::instance();
    std::ostringstream oss;

    oss << "create table if not exists " << TableName << "(";
    for (int i=0; i<EoC; ++i)
    {
        oss << ColumnName[i] << " " << ColumnMeta[i];
        if (i != EoC-1)
        {
            oss << ", ";
        }
    }
    oss << ");";

    rdbConnection.exec(oss.str(), true);
}

bool ItemLogger::insertItem(
    int orderId,
    const std::string& itemId,
    double price,
    int quantity,
    bool isRec
)
{
    if (orderId <= 0 || itemId.empty())
    {
        LOG(ERROR) << "item log needs both valid order id and item id";
        return false;
    }

    std::ostringstream oss;
    oss << "insert into " << TableName << " values("
    << orderId << ", "
    << "\"" << itemId << "\", "
    << price << ", "
    << quantity << ", "
    << isRec << ");";

    if (!RDbConnection::instance().exec(oss.str()))
        return false;

    return true;
}

}
