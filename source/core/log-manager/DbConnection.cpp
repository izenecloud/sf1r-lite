#include "DbConnection.h"

using namespace std;

namespace sf1r {

DbConnection::DbConnection()
{
    sqlKeywords_[ATTR_AUTO_INCREMENT] = "AUTOINCREMENT";
    sqlKeywords_[FUNC_LAST_INSERT_ID] = "last_insert_rowid()";
}

DbConnection::~DbConnection()
{
    close();
}

void DbConnection::close()
{
    mutex_.acquire_write_lock();
    for(list<sqlite3*>::iterator it= pool_.begin(); it!=pool_.end(); it++ ) {
        sqlite3_close(*it);
    }
    pool_.clear();
    mutex_.release_write_lock();
}

bool DbConnection::init(const std::string & path )
{
    bool ret = true;
    mutex_.acquire_write_lock();
    for(int i=0; i<PoolSize; i++) {
        sqlite3* db;
        int rc = sqlite3_open(path.c_str(), &db);
        if ( rc ) {
            cerr << "[LogManager] could not open database: " << sqlite3_errmsg(db) << endl;
            ret = false;
            break;
        }

        sqlite3_busy_handler(db, &DbConnection::busyhandler , NULL);

        char* zErrMsg;
        if( sqlite3_exec(db, "PRAGMA synchronous=OFF;", NULL, NULL, &zErrMsg) != SQLITE_OK ||
           sqlite3_exec(db, "PRAGMA journal_mode = OFF;", NULL, NULL, &zErrMsg) != SQLITE_OK ) {
            cerr << "[LogManager] fail to initialize: " << zErrMsg << endl;
            sqlite3_free(zErrMsg);
            ret = false;
            break;
        }

        pool_.push_back(db);
    }
    mutex_.release_write_lock();
    if(!ret) close();
    return ret;
}

sqlite3* DbConnection::getDb()
{
    sqlite3* db = NULL;
    mutex_.acquire_write_lock();
    if(!pool_.empty()) {
        db = pool_.front();
        pool_.pop_front();
    }
    mutex_.release_write_lock();
    return db;
}

void DbConnection::putDb(sqlite3* db)
{
    mutex_.acquire_write_lock();
    pool_.push_back(db);
    mutex_.release_write_lock();
}

bool DbConnection::exec(const std::string & sql, bool omitError)
{
    char *zErrMsg = NULL;
    sqlite3* db = getDb();
    if( db == NULL ) {
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

bool DbConnection::exec(const std::string & sql,
    std::list< std::map<std::string, std::string> > & results,
    bool omitError)
{
    char *zErrMsg = NULL;
    sqlite3* db = getDb();
    if( db == NULL ) {
        cerr << "[LogManager] No available connection in pool" << endl;
        return false;
    }

    int rc = sqlite3_exec(db, sql.c_str(), &DbConnection::callback, &results, &zErrMsg);
    putDb(db);
    if ( rc != SQLITE_OK && !omitError )
    {
        cerr << "[LogManager] Execute SQL[" << sql << "] error: " << zErrMsg << endl;
        sqlite3_free(zErrMsg);
        return false;
    }
    return true;
}

int DbConnection::busyhandler(void *data, int times)
{
    if(times >= 256) {
        return 0;
    } else {
        sqlite3_sleep(23);
        return 1;
    }
}

int DbConnection::callback(void *data, int argc,
    char **argv, char **azColName)
{
    list< map<string,string> >* resultListPtr =
        (list< map<string,string> >*) data;

    map<string,string> result;
    for(int i=0; i<argc; i++){
        string colName(azColName[i]);
        if( argv[i] ) {
            string colValue(argv[i]);
            result[colName] = colValue;
        }
    }
    if(resultListPtr)
        resultListPtr->push_back(result);
    return 0;
}

}
