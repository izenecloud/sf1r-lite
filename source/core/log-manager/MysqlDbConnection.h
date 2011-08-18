#ifndef SF1R_MYSQL_DBCONNECTION_H_
#define SF1R_MYSQL_DBCONNECTION_H_

#include <iostream>
#include <string>
#include <map>
#include <list>

#include <mysql/mysql.h>

#include <util/ThreadModel.h>
#include "DbConnectionBase.h"

namespace sf1r {

class MysqlDbConnection : public DbConnectionBase
{
public:

    MysqlDbConnection();

    ~MysqlDbConnection();

    bool init(const std::string& str );

    /// close all database connections
    void close();

    /// @sql SQL command
    /// @return no result
    /// @throw exception if underlying database reports error
    bool exec(const std::string & sql, bool omitError=false);

    /// @sql SQL command
    /// @results execution results of executing the SQL command
    /// @throw exception if underlying database reports error
    bool exec(const std::string & sql, std::list< std::map<std::string, std::string> > & results, bool omitError=false);

private:
    
    const std::string DB_NAME;
    
    const int PoolSize;

    MYSQL* getDb();

    void putDb(MYSQL *);

    izenelib::util::ReadWriteLock mutex_;

    std::list<MYSQL*> pool_;

};


}

#endif
