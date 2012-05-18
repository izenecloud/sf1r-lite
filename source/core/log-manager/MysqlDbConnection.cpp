#include "MysqlDbConnection.h"
#include "MysqlDbPath.h"

#include <vector>
#include <glog/logging.h>

namespace sf1r
{

MysqlDbConnection::MysqlDbConnection()
        :PoolSize(16)
{
    sqlKeywords_[RDbConnection::ATTR_AUTO_INCREMENT] = "AUTO_INCREMENT";
    sqlKeywords_[RDbConnection::FUNC_LAST_INSERT_ID] = "last_insert_id()";
}

MysqlDbConnection::~MysqlDbConnection()
{
    close();
}

void MysqlDbConnection::close()
{
    boost::unique_lock<boost::mutex> lock(mutex_);
    for (std::list<MYSQL*>::iterator it= pool_.begin(); it!=pool_.end(); it++ )
    {
        mysql_close(*it);
    }
    pool_.clear();

    //release it at last
    mysql_library_end();
}

bool MysqlDbConnection::init(const std::string& str )
{
    mysql_library_init(0, NULL, NULL);

    MysqlDbPath dbPath;

    if (!dbPath.parse(str) || !dbPath.createDatabase())
        return false;

    boost::unique_lock<boost::mutex> lock(mutex_);
    for (int i=0; i<PoolSize; i++)
    {
        MYSQL* mysql = dbPath.createClient();
        if (!mysql)
            return false;

        pool_.push_back(mysql);
    }

    return true;
}

MYSQL* MysqlDbConnection::getDb()
{
    MYSQL* db = NULL;
    boost::unique_lock<boost::mutex> lock(mutex_);
    while (pool_.empty())
    {
        cond_.wait(lock);
    }
    db = pool_.front();
    pool_.pop_front();
    return db;
}

void MysqlDbConnection::putDb(MYSQL* db)
{
    boost::unique_lock<boost::mutex> lock(mutex_);
    pool_.push_back(db);
    cond_.notify_one();
}

bool MysqlDbConnection::exec(const std::string & sql, bool omitError)
{
    bool ret = true;
    MYSQL* db = getDb();
    if ( db == NULL )
    {
        LOG(ERROR) << "[LogManager] No available connection in pool";
        return false;
    }

    if ( mysql_real_query(db, sql.c_str(), sql.length()) >0 && mysql_errno(db) )
    {
        PRINT_MYSQL_ERROR(db, "in mysql_real_query(): " << sql);
        ret = false;
    }

    putDb(db);
    return ret;
}

bool MysqlDbConnection::exec(const std::string & sql,
                             std::list< std::map<std::string, std::string> > & results,
                             bool omitError)
{
    bool ret = true;
    MYSQL* db = getDb();
    if ( db == NULL )
    {
        LOG(ERROR) << "[LogManager] No available connection in pool";
        return false;
    }

    if ( mysql_real_query(db, sql.c_str(), sql.length()) >0 && mysql_errno(db) )
    {
        PRINT_MYSQL_ERROR(db, "in mysql_real_query(): " << sql);
        ret = false;
    }
    else
    {
        ret = fetchRows_(db, results);
    }

    putDb(db);
    return ret;
}

bool MysqlDbConnection::fetchRows_(MYSQL* db, std::list< std::map<std::string, std::string> > & rows)
{
    bool success = true;
    int status = 0;
    while (status == 0)
    {
        MYSQL_RES* result = mysql_store_result(db);
        if (result)
        {
            uint32_t num_cols = mysql_num_fields(result);
            std::vector<std::string> col_nums(num_cols);
            for (uint32_t i=0;i<num_cols;i++)
            {
                MYSQL_FIELD* field = mysql_fetch_field_direct(result, i);
                col_nums[i] = field->name;
            }
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(result)))
            {
                std::map<std::string, std::string> map;
                for (uint32_t i=0;i<num_cols;i++)
                {
                    map.insert(std::make_pair(col_nums[i], row[i]) );
                }
                rows.push_back(map);
            }
            mysql_free_result(result);
        }
        else
        {
            if (mysql_field_count(db))
            {
                PRINT_MYSQL_ERROR(db, "in mysql_field_count()");
                success = false;
                break;
            }
        }

        // more results? -1 = no, >0 = error, 0 = yes (keep looping)
        if ((status = mysql_next_result(db)) > 0)
        {
            PRINT_MYSQL_ERROR(db, "in mysql_next_result()");
            success = false;
            break;
        }
    }

    return success;
}

}
