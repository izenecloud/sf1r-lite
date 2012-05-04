#ifndef SF1R_MYSQL_DB_PATH_H_
#define SF1R_MYSQL_DB_PATH_H_

#include <mysql/mysql.h>
#include <string>
#include <glog/logging.h>

namespace sf1r
{

struct MysqlDbPath
{
    std::string username;
    std::string password;
    std::string host;
    uint32_t port;
    std::string database;

    MysqlDbPath();

    /**
     * @param pathStr path string such as "root:123456@127.0.0.1:3306/SF1R"
     */
    bool parse(const std::string& pathStr);

    bool createDatabase() const;

    MYSQL* createClient() const;
};

#define PRINT_MYSQL_ERROR(mysql, message)               \
    LOG(ERROR) << "Error: " << mysql_errno(mysql)       \
               << " [" << mysql_error(mysql) << "] "    \
               << message;

} // namespace sf1r

#endif // SF1R_MYSQL_DB_PATH_H_
