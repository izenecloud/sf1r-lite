#include "Sqlite3DbConnection.h"
#include <boost/filesystem.hpp>
using namespace std;

namespace sf1r
{

Sqlite3DbConnection::Sqlite3DbConnection()
{
    sqlKeywords_[RDbConnection::ATTR_AUTO_INCREMENT] = "AUTOINCREMENT";
    sqlKeywords_[RDbConnection::FUNC_LAST_INSERT_ID] = "last_insert_rowid()";
}

Sqlite3DbConnection::~Sqlite3DbConnection()
{
    close();
}

void Sqlite3DbConnection::close()
{
    boost::unique_lock<boost::mutex> lock(mutex_);
    for (list<sqlite3*>::iterator it= pool_.begin(); it!=pool_.end(); it++ )
    {
        sqlite3_close(*it);
    }
    pool_.clear();
}

bool Sqlite3DbConnection::init(const std::string & path )
{
    boost::filesystem::path log_path(path);
    boost::filesystem::path log_dir = log_path.parent_path();
    if (boost::filesystem::exists(log_dir))
    {
        if (!boost::filesystem::is_directory(log_dir))
        {
            std::cerr << "Log Directory " << log_dir << " is a file" << std::endl;
            return false;
        }
    }
    else
    {
        boost::filesystem::create_directories(log_dir);
    }
    bool ret = true;
    boost::unique_lock<boost::mutex> lock(mutex_);

    for (int i=0; i<PoolSize; i++)
    {
        sqlite3* db;
        int rc = sqlite3_open(path.c_str(), &db);
        if ( rc )
        {
            cerr << "[LogManager] could not open database: " << sqlite3_errmsg(db) << endl;
            ret = false;
            break;
        }

        sqlite3_busy_handler(db, &Sqlite3DbConnection::busyhandler , NULL);

        char* zErrMsg;
        if ( sqlite3_exec(db, "PRAGMA synchronous=OFF;", NULL, NULL, &zErrMsg) != SQLITE_OK ||
                sqlite3_exec(db, "PRAGMA journal_mode = OFF;", NULL, NULL, &zErrMsg) != SQLITE_OK )
        {
            cerr << "[LogManager] fail to initialize: " << zErrMsg << endl;
            sqlite3_free(zErrMsg);
            ret = false;
            break;
        }

        pool_.push_back(db);
    }
    if (!ret) close();
    return ret;
}

sqlite3* Sqlite3DbConnection::getDb()
{
    sqlite3* db = NULL;
    boost::unique_lock<boost::mutex> lock(mutex_);
    while (pool_.empty())
    {
        cond_.wait(lock);
    }
    db = pool_.front();
    pool_.pop_front();

    return db;
}

void Sqlite3DbConnection::putDb(sqlite3* db)
{
    boost::unique_lock<boost::mutex> lock(mutex_);
    pool_.push_back(db);
    cond_.notify_one();
}

bool Sqlite3DbConnection::exec(const std::string & sql, bool omitError)
{
    char *zErrMsg = NULL;
    sqlite3* db = getDb();
    if ( db == NULL )
    {
        cerr << "[LogManager] No available connection in pool" << endl;
        return false;
    }

    int rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, &zErrMsg);
    putDb(db);
    if ( rc != SQLITE_OK && !omitError )
    {
        cerr << "[LogManager] Execute SQL[" << sql << "] error: " << zErrMsg << endl;
        sqlite3_free(zErrMsg);
        return false;
    }
    return true;
}

bool Sqlite3DbConnection::exec(const std::string & sql,
                               std::list< std::map<std::string, std::string> > & results,
                               bool omitError)
{
    char *zErrMsg = NULL;
    sqlite3* db = getDb();
    if ( db == NULL )
    {
        cerr << "[LogManager] No available connection in pool" << endl;
        return false;
    }

    int rc = sqlite3_exec(db, sql.c_str(), &Sqlite3DbConnection::callback, &results, &zErrMsg);
    putDb(db);
    if ( rc != SQLITE_OK && !omitError )
    {
        cerr << "[LogManager] Execute SQL[" << sql << "] error: " << zErrMsg << endl;
        sqlite3_free(zErrMsg);
        return false;
    }
    return true;
}

int Sqlite3DbConnection::busyhandler(void *data, int times)
{
    if (times >= 256)
    {
        return 0;
    }
    else
    {
        sqlite3_sleep(23);
        return 1;
    }
}

int Sqlite3DbConnection::callback(void *data, int argc,
                                  char **argv, char **azColName)
{
    list< map<string,string> >* resultListPtr =
        (list< map<string,string> >*) data;

    map<string,string> result;
    for (int i=0; i<argc; i++)
    {
        string colName(azColName[i]);
        if ( argv[i] )
        {
            string colValue(argv[i]);
            result[colName] = colValue;
        }
    }
    if (resultListPtr)
        resultListPtr->push_back(result);
    return 0;
}

}
