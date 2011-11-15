#ifndef SF1R_RDBCONNECTIONBASE_H_
#define SF1R_RDBCONNECTIONBASE_H_

#include "RDbConnection.h" // SQL_KEYWORD
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#include <iostream>
#include <string>

namespace sf1r {

class RDbConnectionBase
{
public:

    RDbConnectionBase(){}

    virtual ~RDbConnectionBase(){}

    virtual bool init(const std::string& str){return true;}

    virtual void close(){}

    /// @sql SQL command
    /// @return no result
    /// @throw exception if underlying database reports error
    virtual bool exec(const std::string & sql, bool omitError=false){return true;}

    /// @sql SQL command
    /// @results execution results of executing the SQL command
    /// @throw exception if underlying database reports error
    virtual bool exec(const std::string & sql, std::list< std::map<std::string, std::string> > & results, bool omitError=false){return true;}

    const std::string& getSqlKeyword(RDbConnection::SQL_KEYWORD type) const {
        return sqlKeywords_[type];
    }

protected:
    std::string sqlKeywords_[RDbConnection::SQL_KEYWORD_NUM];
};

}

#endif
