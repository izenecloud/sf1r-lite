/**
 * @file ItemLogger.h
 * @author Jun Jiang
 * @date 2011-08-31
 */

#ifndef _ITEM_LOGGER_H
#define _ITEM_LOGGER_H

#include "LogManagerSingleton.h"

#include <string>

namespace sf1r
{

class ItemLogger : public LogManagerSingleton<ItemLogger>
{
public:
    enum Column
    {
        OrderId, // reference the column "Id" in OrderLogger
        ItemId,
        Price,
        Quantity,
        IsRec,
        EoC
    };

    static const char* ColumnName[EoC];
    static const char* ColumnMeta[EoC];
    static const char* TableName;

    static void createTable();

    /**
     * @return true for success, false for failure
     */
    bool insertItem(
        int orderId,
        const std::string& itemId,
        double price,
        int quantity,
        bool isRec
    );
};

} // namespace sf1r

#endif // _ITEM_LOGGER_H
