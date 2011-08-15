#ifndef SF1R__DBCONNECTIONBASE_H_
#define SF1R__DBCONNECTIONBASE_H_

#include <iostream>
#include <string>

namespace sf1r {

class DbConnectionBase
{
public:

    DbConnectionBase();

    virtual ~DbConnectionBase();

    
    virtual bool init(const std::string& str );

    virtual void close();

    /// @sql SQL command
    /// @return no result
    /// @throw exception if underlying database reports error
    virtual bool exec(const std::string & sql, bool omitError=false);

    /// @sql SQL command
    /// @results execution results of executing the SQL command
    /// @throw exception if underlying database reports error
    virtual bool exec(const std::string & sql, std::list< std::map<std::string, std::string> > & results, bool omitError=false);

};

}

#endif
