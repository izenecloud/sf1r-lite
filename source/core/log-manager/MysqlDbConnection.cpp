#include "MysqlDbConnection.h"

#include <vector>

#include <boost/lexical_cast.hpp>

#include <cstdio>

namespace sf1r {

MysqlDbConnection::MysqlDbConnection()
:DB_NAME("SF1R"), PoolSize(16)
{
}

MysqlDbConnection::~MysqlDbConnection()
{
    close();
}

void MysqlDbConnection::close()
{
    mutex_.acquire_write_lock();
    for(std::list<MYSQL*>::iterator it= pool_.begin(); it!=pool_.end(); it++ ) {
        mysql_close(*it);
    }
    pool_.clear();
    
    //release it at last
    mysql_library_end();
    mutex_.release_write_lock();
}

bool MysqlDbConnection::init(const std::string& str )
{
    mysql_library_init(0, NULL, NULL);
    std::string host;
    uint32_t port = 3306;
    std::string username;
    std::string password;
    std::string schema = DB_NAME;
    std::string default_charset("utf8");
//     std::string character_set_results("utf8");
    
    std::string conn = str;
    std::size_t pos = conn.find(":");
    if(pos==0 || pos == std::string::npos) return false;
    username = conn.substr(0, pos);
    conn = conn.substr(pos+1);
    pos = conn.find("@");
    if(pos==0 || pos == std::string::npos) return false;
    password = conn.substr(0, pos);
    conn = conn.substr(pos+1);
    pos = conn.find(":");
    if(pos==0 || pos == std::string::npos) return false;
    host = conn.substr(0, pos);
    conn = conn.substr(pos+1);
    try
    {
        port = boost::lexical_cast<uint32_t>(conn);
    }
    catch(std::exception& ex)
    {
        return false;
    }
    if(host=="localhost") host = "127.0.0.1";
    int flags = CLIENT_MULTI_RESULTS;
    
    
    bool ret = true;
    mutex_.acquire_write_lock();
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
            printf("Couldn't connect mysql : %d:(%s) %s", mysql_errno(mysql), mysql_sqlstate(mysql), mysql_error(mysql));
            mysql_close(mysql);
            return false;
        }
        
        mysql_query(mysql, "SET NAMES utf8");
        
        std::string create_db_query = "create database IF NOT EXISTS "+schema+" default character set utf8";
        if ( mysql_query(mysql, create_db_query.c_str())>0 ) {
            printf("Error %u: %s\n", mysql_errno(mysql), mysql_error(mysql));
            mysql_close(mysql);
            return false;
        }
        
        std::string use_db_query = "use "+schema;
        if ( mysql_query(mysql, use_db_query.c_str())>0 ) {
            printf("Error %u: %s\n", mysql_errno(mysql), mysql_error(mysql));
            mysql_close(mysql);
            return false;
        }

        pool_.push_back(mysql);
    }
    mutex_.release_write_lock();
    if(!ret) close();
    return ret;
}

MYSQL* MysqlDbConnection::getDb()
{
    MYSQL* db = NULL;
    mutex_.acquire_write_lock();
    if(!pool_.empty()) {
        db = pool_.front();
        pool_.pop_front();
    }
    mutex_.release_write_lock();
    return db;
}

void MysqlDbConnection::putDb(MYSQL* db)
{
    mutex_.acquire_write_lock();
    pool_.push_back(db);
    mutex_.release_write_lock();
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
        printf("Error : %d:(%s) %s", mysql_errno(db), mysql_sqlstate(db), mysql_error(db));
        ret = false; 
    }
//     if(ret)
//     {
//         uint32_t field_count = mysql_field_count(db);
//         std::cout<<"mysql exec field_count : "<<field_count<<std::endl;
//         if ( field_count<= 0) ret = false;
//     }
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
        printf("Error : %d:(%s) %s", mysql_errno(db), mysql_sqlstate(db), mysql_error(db));
        ret = false; 
    }
    else
    {
        MYSQL_RES* result = mysql_store_result(db);
        if(result==NULL)
        {
            printf("Error during store_result : %d:(%s) %s", mysql_errno(db), mysql_sqlstate(db), mysql_error(db));
            ret = false; 
        }
        else
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
                results.push_back(map);
            }
        }
    }

    putDb(db);
    return ret;
}


}
