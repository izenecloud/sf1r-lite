#ifndef SF1R__DBCONNECTIONBASE_H_
#define SF1R__DBCONNECTIONBASE_H_

#include "DbConnection.h" // SQL_KEYWORD

#include <iostream>
#include <string>

namespace sf1r {

class DbConnectionBase
{
public:

    DbConnectionBase(){}

    virtual ~DbConnectionBase(){}

    
    virtual bool init(const std::string& str ){return true;}

    virtual void close(){}

    /// @sql SQL command
    /// @return no result
    /// @throw exception if underlying database reports error
    virtual bool exec(const std::string & sql, bool omitError=false){return true;}

    /// @sql SQL command
    /// @results execution results of executing the SQL command
    /// @throw exception if underlying database reports error
    virtual bool exec(const std::string & sql, std::list< std::map<std::string, std::string> > & results, bool omitError=false){return true;}

    const std::string& getSqlKeyword(DbConnection::SQL_KEYWORD type) const {
        return sqlKeywords_[type];
    }

protected:
    std::string sqlKeywords_[DbConnection::SQL_KEYWORD_NUM];
};

}

#endif
