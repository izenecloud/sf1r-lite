#include "MysqlDbPath.h"

#include <sstream>
#include <boost/lexical_cast.hpp>

namespace sf1r
{

MysqlDbPath::MysqlDbPath()
    : port(3306)
    , database("SF1R")
{}

bool MysqlDbPath::parse(const std::string& pathStr)
{
    std::string path = pathStr;

    std::size_t pos = path.find(":");
    if (pos==0 || pos == std::string::npos)
        return false;

    username = path.substr(0, pos);
    path = path.substr(pos+1);
    pos = path.find("@");
    if (pos == std::string::npos)
        return false;

    password = path.substr(0, pos);
    path = path.substr(pos+1);
    pos = path.find(":");
    if (pos==0 || pos == std::string::npos)
        return false;

    host = path.substr(0, pos);
    path = path.substr(pos+1);
    pos = path.find("/");
    if (pos==0)
        return false;

    std::string portStr;
    if (pos == std::string::npos)
    {
        portStr = path;
    }
    else
    {
        portStr = path.substr(0, pos);
        path = path.substr(pos+1);
        if (!path.empty())
            database = path;
    }

    try
    {
        port = boost::lexical_cast<uint32_t>(portStr);
    }
    catch (std::exception& ex)
    {
        return false;
    }

    if (host == "localhost")
        host = "127.0.0.1";

    return true;
}

bool MysqlDbPath::createDatabase() const
{
    MYSQL* mysql = mysql_init(NULL);
    if (!mysql)
    {
        LOG(ERROR) << "mysql_init failed";
        return false;
    }

    if (!mysql_real_connect(mysql, host.c_str(),
                            username.c_str(), password.c_str(),
                            NULL, port, NULL, 0))
    {
        PRINT_MYSQL_ERROR(mysql, "in mysql_real_connect()");
        mysql_close(mysql);
        return false;
    }

    std::ostringstream oss;
    oss << "create database IF NOT EXISTS " << database
        << " default character set utf8";
    std::string sqlStr = oss.str();

    if (mysql_query(mysql, sqlStr.c_str()))
    {
        PRINT_MYSQL_ERROR(mysql, "in mysql_query(): " << sqlStr);
        mysql_close(mysql);
        return false;
    }

    mysql_close(mysql);
    return true;
}

MYSQL* MysqlDbPath::createClient() const
{
    MYSQL* mysql = mysql_init(NULL);
    if (!mysql)
    {
        LOG(ERROR) << "mysql_init failed";
        return NULL;
    }

    mysql_options(mysql, MYSQL_SET_CHARSET_NAME, "utf8");

    my_bool reconnect = 1;
    if (mysql_options(mysql, MYSQL_OPT_RECONNECT, &reconnect))
    {
        PRINT_MYSQL_ERROR(mysql, "in mysql_options()");
        mysql_close(mysql);
        return NULL;
    }

    if (mysql_options(mysql, MYSQL_INIT_COMMAND, "SET NAMES utf8"))
    {
        PRINT_MYSQL_ERROR(mysql, "in mysql_options()");
        mysql_close(mysql);
        return NULL;
    }

    const int flags = CLIENT_MULTI_STATEMENTS;

    if (!mysql_real_connect(mysql, host.c_str(),
                            username.c_str(), password.c_str(),
                            database.c_str(), port, NULL, flags))
    {
        PRINT_MYSQL_ERROR(mysql, "in mysql_real_connect()");
        mysql_close(mysql);
        return NULL;
    }

    return mysql;
}

} // namespace sf1r
