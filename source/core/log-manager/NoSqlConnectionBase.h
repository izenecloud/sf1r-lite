#ifndef SF1R_NOSQL_CONNECTION_BASE_H_
#define SF1R_NOSQL_CONNECTION_BASE_H_

#include "NoSqlConnection.h" // SQL_KEYWORD

#include <string>

namespace sf1r {

class NoSqlConnectionBase
{
public:

    NoSqlConnectionBase(){}

    virtual ~NoSqlConnectionBase(){}

    virtual bool init(const std::string& str){return true;}

    virtual void close(){}
};

}

#endif
