#ifndef _NOSQL_CONNECTION_H_
#define _NOSQL_CONNECTION_H_

#include <string>
#include <map>
#include <list>

#include <util/ThreadModel.h>

#include "LogManagerSingleton.h"

namespace sf1r {

class NoSqlConnectionBase;

class NoSqlConnection : public LogManagerSingleton<NoSqlConnection>
{
public:
    NoSqlConnection();

    ~NoSqlConnection();

    bool init(const std::string& str);

    /// close all database connections
    void close();

private:
    NoSqlConnectionBase* impl_;
};

}

#endif
