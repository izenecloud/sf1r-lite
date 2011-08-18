#include "DbConnection.h"
#include "DbConnectionBase.h"
#include "Sqlite3DbConnection.h"
#include "MysqlDbConnection.h"

#include <boost/algorithm/string.hpp>
using namespace std;

namespace sf1r {

DbConnection::DbConnection()
:impl_(NULL)
{
}

DbConnection::~DbConnection()
{
    if(impl_)
    {
        delete impl_;
    }
}

void DbConnection::close()
{
    if(impl_)
    {
        impl_->close();
    }
}

bool DbConnection::init(const std::string& str )
{
    std::string sqlite3_prefix = "sqlite3://";
    std::string mysql_prefix = "mysql://";
    if(boost::algorithm::starts_with(str, sqlite3_prefix))
    {
        impl_ = new Sqlite3DbConnection();
        std::string path = str.substr(sqlite3_prefix.length());
        return impl_->init(path);
    }
    else if(boost::algorithm::starts_with(str, mysql_prefix))
    {
        impl_ = new MysqlDbConnection();
        std::string mysql_str = str.substr(mysql_prefix.length());
        return impl_->init(mysql_str);
    }
    else
    {
        std::cerr<<"[DbConnection::init] "<<str<<" unrecognized"<<std::endl;
        return false;
    }
    
}


bool DbConnection::exec(const std::string & sql, bool omitError)
{
    return impl_->exec(sql, omitError);
}

bool DbConnection::exec(const std::string & sql,
    std::list< std::map<std::string, std::string> > & results,
    bool omitError)
{
    return impl_->exec(sql, results, omitError);
}


}
