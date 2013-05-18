#include "RDbConnection.h"
#include "RDbConnectionBase.h"
#include "Sqlite3DbConnection.h"
#include "MysqlDbConnection.h"
#include "LogAnalysisConnection.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
using namespace std;

namespace sf1r {

RDbConnection::RDbConnection()
:impl_(NULL),
logserver_(false)
{
}

RDbConnection::~RDbConnection()
{
    if(impl_)
    {
        delete impl_;
    }
}

void RDbConnection::close()
{
    if(impl_)
    {
        impl_->close();
    }
}

bool RDbConnection::init(const std::string& str )
{
    std::string sqlite3_prefix = "sqlite3://";
    std::string mysql_prefix = "mysql://";
    std::string logserver_prefix = "logserver://";
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
    else if(boost::algorithm::starts_with(str, logserver_prefix))
    {

        logserver_ = true;
        LogServerConnectionConfig lscfg;
        std::vector<std::string> strs;
        std::string conf = str.substr(logserver_prefix.length());
        boost::split(strs, conf, boost::is_any_of(","));
        if(strs.size() != 4)
            std::cerr <<"[LogAnalysisConnection::init] "<<str <<" error"<<endl;
        lscfg.host = strs[0];
        lscfg.rpcPort = boost::lexical_cast<unsigned int>(strs[1]);
        lscfg.rpcThreadNum = boost::lexical_cast<unsigned int>(strs[2]);
        lscfg.driverPort = boost::lexical_cast<unsigned int>(strs[3]);
        LogAnalysisConnection::instance().init(lscfg);
        return true;
    }
    else
    {
        std::cerr<<"[RDbConnection::init] "<<str<<" unrecognized"<<std::endl;
        return false;
    }

}

bool RDbConnection::exec(const std::string & sql, bool omitError)
{
    return impl_->exec(sql, omitError);
}

bool RDbConnection::exec(const std::string & sql,
    std::list< std::map<std::string, std::string> > & results,
    bool omitError)
{
    return impl_->exec(sql, results, omitError);
}

const std::string& RDbConnection::getSqlKeyword(SQL_KEYWORD type) const
{
    return impl_->getSqlKeyword(type);
}

}
