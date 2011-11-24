/**
 * @file OrderLogger.h
 * @author Jun Jiang
 * @date 2011-08-29
 */

#ifndef _ORDER_LOGGER_H
#define _ORDER_LOGGER_H

#include "LogManagerSingleton.h"

#include <string>
#include <list>
#include <map>

namespace sf1r
{

class OrderLogger : public LogManagerSingleton<OrderLogger>
{
public:
    enum Column
    {
        Id, // the primary key which is automatically incremented
        StrId, // the string id supplied by customer
        Collection,
        UserId,
        TimeStamp,
        EoC
    };

    static const char* ColumnName[EoC];
    static const char* ColumnMeta[EoC];
    static const char* TableName;

    static void createTable();

    /**
     * @param date format "YYYY-mm-dd"
     * @param newId return the unique id if the order is inserted successfully
     * @return true for success, false for failure
     */
    bool insertOrder(
        const std::string& orderId,
        const std::string& collection,
        const std::string& userId,
        const std::string& date,
        int& newId
    );

private:
    void prepareSql_(
        const std::string& orderId,
        const std::string& collection,
        const std::string& userId,
        const std::string& date,
        std::string& sql
    ) const;

    const std::string& getLastIdFunc_() const;

    typedef std::map<std::string, std::string> Row;
    typedef std::list<Row> RowList;

    bool getLastIdFromRows_(
        const RowList& selectRows,
        int& lastId
    ) const;
};

} // namespace sf1r

#endif // _ORDER_LOGGER_H
