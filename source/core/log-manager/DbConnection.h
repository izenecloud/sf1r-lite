#ifndef _DB_CONNECTION_H_
#define _DB_CONNECTION_H_

#include <iostream>
#include <string>
#include <map>
#include <list>

#include <sqlite3.h>

#include <util/ThreadModel.h>

#include "LogManagerSingleton.h"

namespace sf1r {

class DbConnection : public LogManagerSingleton<DbConnection>
{
public:

    DbConnection();

    ~DbConnection();

    /// @path where database file locates
    bool init(const std::string & path );

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

    static const int PoolSize = 16;

    /// callback functions required by sqlite3_exec
    static int callback(void *data, int argc, char **argv, char **azColName);

    static int busyhandler(void *data, int times);

    sqlite3* getDb();

    void putDb(sqlite3 *);

    izenelib::util::ReadWriteLock mutex_;

    std::list<sqlite3*> pool_;

};

}

#endif
