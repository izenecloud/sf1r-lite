/*
*    @file DbTable.hpp
*    @author Kuilong Liu
*    @date 2013.01.07
*/
#ifndef _LEVELDB_TABLE_HPP_
#define _LEVELDB_TABLE_HPP_

#include <types.h>
#include <stdio.h>
#include <string>

#include <am/leveldb/Table.h>
#include <boost/filesystem.hpp>

using namespace izenelib::am;
using namespace izenelib::am::leveldb;
using namespace std;

namespace sf1r
{
/*
*    @class TitleIdDbTable
*/
    class TitleIdDbTable
    {
        typedef Table<std::string, int> TitleIdDbType;
        std::string path_;
        TitleIdDbType *titleIdDbTable;
    public:
        TitleIdDbTable(const std::string& path)
            :path_(path)
            ,titleIdDbTable(NULL)
        {
        }

        ~TitleIdDbTable()
        {
            delete titleIdDbTable;
        }

        /*
         * @brief open db table
         */
        bool open()
        {
            titleIdDbTable = new TitleIdDbType(path_);
            return titleIdDbTable->open();
        }

        void close()
        {
            titleIdDbTable->close();
        }

        void clear()
        {
            titleIdDbTable->clear();
        }

        /*
         * @brief insert a k-v into db
         * @param title (key)
         * @param id    (value)
         */
        bool add_item(const std::string& title, const int& id)
        {
            return titleIdDbTable->insert(title, id);
        }

        bool get_item(const std::string& title, int& id)
        {
            return titleIdDbTable->get(title,id);
        }

        bool delete_item(const std::string& title)
        {
            return titleIdDbTable->del(title);
        }
    };

/*
*    @class IdTitleDbTable
*/
    class IdTitleDbTable
    {
        typedef Table<int, std::string> IdTitleDbType;
        std::string path_;
        IdTitleDbType *idTitleDbTable;
    public:
        IdTitleDbTable(const std::string& path)
            :path_(path)
            ,idTitleDbTable(NULL)
        {
        }

        ~IdTitleDbTable()
        {
            delete idTitleDbTable;
        }

        /*
         * @brief open db table
         */
        bool open()
        {
            idTitleDbTable = new IdTitleDbType(path_);
            return idTitleDbTable->open();
        }

        void close()
        {
            idTitleDbTable->close();
        }

        void clear()
        {
            idTitleDbTable->clear();
        }

        /*
         * @brief insert a k-v into db
         * @param id     (key)
         * @param title  (value)
         */
        bool add_item(const int& id, const std::string& title)
        {
            return idTitleDbTable->insert(id, title);
        }

        bool get_item(const int& id, std::string& title)
        {
            return idTitleDbTable->get(id, title);
        }

        bool delete_item(const int& id)
        {
            return idTitleDbTable->del(id);
        }
    };

}
#endif
