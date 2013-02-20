/**
   @file word_competion_table.hpp
   @author Hongliang Zhao
   @date 2012.07.15
*/
#ifndef _WORD_LEVELDB_TABLE_HPP_
#define _WORD_LEVELDB_TABLE_HPP_

#include <types.h>
#include <stdio.h>
#include <string>

#include <boost/thread/mutex.hpp>
#include <boost/filesystem.hpp>

#include <util/ustring/UString.h>
#include <mining-manager/util/LabelDistance.h>
#include <am/leveldb/Table.h>
#include <am/range/AmIterator.h>

using namespace izenelib::am::leveldb;
using namespace izenelib::am;
using namespace std;
namespace sf1r
{

/**
 * @class WordLevelDBTable
 **/
class WordLevelDBTable
{
    typedef uint32_t FREQ_TYPE; 
    typedef uint8_t CHAR_TYPE;
    typedef Table<std::string, std::string> LevelDBType;
    LevelDBType *dbTable;

public:
    WordLevelDBTable()
    {
        dbTable = NULL;
    }

    ~WordLevelDBTable()
    {
        delete dbTable;
    }

    bool open(string path)// open dbTable;
    {
        dbTable = new LevelDBType(path);
        return dbTable->open();
    }
    void close()
    {
    //dbTable->clear();
        dbTable->close();
    }
    void clear()
    {
        dbTable->clear();
    }
    /*
    *@brief insert a k-v and freq into db;
    *@param key 
    *@param value 
    *@param freq freq of value; 
    */
    bool add_item(const std::string& key, const std::string& value)
    {
        return dbTable->insert(key, value);
    }

    bool add_item(const izenelib::util::UString& key, const izenelib::util::UString& value)
    {
        std::string strkey, strvalue;
        key.convertString(strkey, izenelib::util::UString::UTF_8);
        value.convertString(strvalue, izenelib::util::UString::UTF_8);
        return add_item(strkey, strvalue);
    }

    bool get_item(const izenelib::util::UString& query, std::string &list)
    {
        std::string strkey;
        query.convertString(strkey, izenelib::util::UString::UTF_8);
        return get_item(strkey, list);
    }

    bool get_item(const std::string query, std::string& list)
    {
        return dbTable->get(query, list);
    }

    bool delete_item(const std::string query)
    {
        return dbTable->del(query);
    }

};
}
#endif
