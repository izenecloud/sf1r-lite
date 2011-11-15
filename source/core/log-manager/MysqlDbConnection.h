#ifndef SF1R_MYSQL_DBCONNECTION_H_
#define SF1R_MYSQL_DBCONNECTION_H_

#include <mysql/mysql.h>

#include "RDbConnectionBase.h"

namespace sf1r {

class MysqlDbConnection : public RDbConnectionBase
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
    /// @return true for success, false for failure
    bool fetchRows_(MYSQL* db, std::list< std::map<std::string, std::string> > & rows);

private:
    const int PoolSize;

    MYSQL* getDb();

    void putDb(MYSQL *);

    boost::mutex mutex_;

    boost::condition_variable cond_;

    std::list<MYSQL*> pool_;

};

}

#endif
