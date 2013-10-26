/*
*    @file DbTable.hpp
*    @author Kuilong Liu
*    @date 2013.01.07
*/
#ifndef _IMG_DUP_LEVELDB_TABLE_HPP_
#define _IMG_DUP_LEVELDB_TABLE_HPP_

#include <types.h>
#include <stdio.h>
#include <string>

#include <am/leveldb/Table.h>
#include <boost/filesystem.hpp>


NS_SF1R_B5M_BEGIN
/*
*    @class DocidImgDbTable
*/
class DocidImgDbTable
{
    typedef izenelib::am::leveldb::Table<uint32_t, std::string> DocidImgDbType;
    std::string path_;
    DocidImgDbType *docidImgDbTable;
public:
    DocidImgDbTable(const std::string& path)
        :path_(path)
        ,docidImgDbTable(NULL)
    {
    }

    ~DocidImgDbTable()
    {
        delete docidImgDbTable;
    }

    /*
     * @brief open db table
     */
    bool open()
    {
        docidImgDbTable = new DocidImgDbType(path_);
        return docidImgDbTable->open();
    }

    void close()
    {
        docidImgDbTable->close();
    }

    void clear()
    {
        docidImgDbTable->clear();
    }

    /*
     * @brief insert a k-v into db
     * @param docid     (key)
     * @param imgurl  (value)
     */
    bool add_item(const uint32_t& docid, const std::string& imgurl)
    {
        return docidImgDbTable->insert(docid, imgurl);
    }

    bool get_item(const uint32_t& docid, std::string& imgurl)
    {
        return docidImgDbTable->get(docid, imgurl);
    }

    bool delete_item(const uint32_t& docid)
    {
        return docidImgDbTable->del(docid);
    }
};

NS_SF1R_B5M_END
#endif
