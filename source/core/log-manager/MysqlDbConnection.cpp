#include "MysqlDbConnection.h"

#include <vector>

#include <boost/lexical_cast.hpp>

#include <cstdio>

namespace sf1r {

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
    for(std::list<MYSQL*>::iterator it= pool_.begin(); it!=pool_.end(); it++ ) {
        mysql_close(*it);
    }
    pool_.clear();

    //release it at last
    mysql_library_end();
}

bool MysqlDbConnection::init(const std::string& str )
{
    mysql_library_init(0, NULL, NULL);
    std::string host;
    std::string portStr;
    uint32_t port = 3306;
    std::string username;
    std::string password;
    std::string database("SF1R");
    std::string default_charset("utf8");

    std::string conn = str;
    std::size_t pos = conn.find(":");
    if(pos==0 || pos == std::string::npos) return false;
    username = conn.substr(0, pos);
    conn = conn.substr(pos+1);
    pos = conn.find("@");
    if(pos == std::string::npos) return false;
    password = conn.substr(0, pos);
    conn = conn.substr(pos+1);
    pos = conn.find(":");
    if(pos==0 || pos == std::string::npos) return false;
    host = conn.substr(0, pos);
    conn = conn.substr(pos+1);
    pos = conn.find("/");
    if(pos==0) return false;
    if(pos == std::string::npos)
    {
        portStr = conn;
    }
    else
    {
        portStr = conn.substr(0, pos);
        conn = conn.substr(pos+1);
        if (!conn.empty())
            database = conn;
    }
    try
    {
        port = boost::lexical_cast<uint32_t>(portStr);
    }
    catch(std::exception& ex)
    {
        return false;
    }
    if(host=="localhost") host = "127.0.0.1";
    int flags = CLIENT_MULTI_STATEMENTS;

    bool ret = true;
    boost::unique_lock<boost::mutex> lock(mutex_);
    for(int i=0; i<PoolSize; i++) {
        MYSQL* mysql = mysql_init(NULL);
        if (!mysql)
        {
            std::cerr<<"mysql_init failed"<<std::endl;
            return false;
        }
        mysql_options(mysql, MYSQL_SET_CHARSET_NAME, default_charset.c_str());

        if (!mysql_real_connect(mysql, host.c_str(), username.c_str(), password.c_str(), NULL, port, NULL, flags))
        {
            fprintf(stderr, "Couldn't connect mysql : %d:(%s) %s\n", mysql_errno(mysql), mysql_sqlstate(mysql), mysql_error(mysql));
            mysql_close(mysql);
            return false;
        }

        mysql_query(mysql, "SET NAMES utf8");

        std::string create_db_query = "create database IF NOT EXISTS "+database+" default character set utf8";
        if ( mysql_query(mysql, create_db_query.c_str())>0 ) {
            fprintf(stderr, "Error %u: %s\n", mysql_errno(mysql), mysql_error(mysql));
            mysql_close(mysql);
            return false;
        }

        std::string use_db_query = "use "+database;
        if ( mysql_query(mysql, use_db_query.c_str())>0 ) {
            fprintf(stderr, "Error %u: %s\n", mysql_errno(mysql), mysql_error(mysql));
            mysql_close(mysql);
            return false;
        }

        pool_.push_back(mysql);
    }
    if(!ret) close();
    return ret;
}

MYSQL* MysqlDbConnection::getDb()
{
    MYSQL* db = NULL;
    boost::unique_lock<boost::mutex> lock(mutex_);
    while(pool_.empty())
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
    if( db == NULL ) {
        std::cerr << "[LogManager] No available connection in pool" << std::endl;
        return false;
    }

    if( mysql_real_query(db, sql.c_str(), sql.length()) >0 && mysql_errno(db) )
    {
        fprintf(stderr, "Error : %d:(%s) %s\n", mysql_errno(db), mysql_sqlstate(db), mysql_error(db));
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
    if( db == NULL ) {
        std::cerr << "[LogManager] No available connection in pool" << std::endl;
        return false;
    }

    if( mysql_real_query(db, sql.c_str(), sql.length()) >0 && mysql_errno(db) )
    {
        fprintf(stderr, "Error : %d:(%s) %s\n", mysql_errno(db), mysql_sqlstate(db), mysql_error(db));
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
        if(result)
        {
            uint32_t num_cols = mysql_num_fields(result);
            std::vector<std::string> col_nums(num_cols);
            for(uint32_t i=0;i<num_cols;i++)
            {
                MYSQL_FIELD* field = mysql_fetch_field_direct(result, i);
                col_nums[i] = field->name;
            }
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(result)))
            {
                std::map<std::string, std::string> map;
                for(uint32_t i=0;i<num_cols;i++)
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
                fprintf(stderr, "Error during store_result : %d:(%s) %s\n", mysql_errno(db), mysql_sqlstate(db), mysql_error(db));
                success = false;
                break;
            }
        }

        // more results? -1 = no, >0 = error, 0 = yes (keep looping)
        if ((status = mysql_next_result(db)) > 0)
        {
            fprintf(stderr, "Error during next_result : %d:(%s) %s\n", mysql_errno(db), mysql_sqlstate(db), mysql_error(db));
            success = false;
            break;
        }
    }

    return success;
}

}
