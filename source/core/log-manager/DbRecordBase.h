#ifndef _DB_RECORD_BASE_H_
#define _DB_RECORD_BASE_H_

#include "DbConnection.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <map>

namespace sf1r {

class DbRecordBase
{

public:

    DbRecordBase() : exist_(false) {}

    virtual ~DbRecordBase();

    static bool find_by_sql(const std::string & sql,
        std::list<std::map<std::string, std::string> > & records)
    {
        if ( ! DbConnection::instance().exec(sql, records) )
        {
            return false;
        }
        return true;
    }

    static bool delete_by_sql(const std::string & sql)
    {
        if( ! DbConnection::instance().exec(sql) )
        {
            return false;
        }
        return true;
    }


    /// Save record into a map
    virtual void save( std::map<std::string, std::string> & ) = 0;

    /// Initialize itself from a map
    virtual void load( const std::map<std::string, std::string> & ) = 0;

protected:

    /// Whether the record exist in database
    bool exist_;

};

template <typename DbRecordType >
void createTable()
{
    std::stringstream sql;

    sql << "create table " << DbRecordType::TableName << "(";
    for ( int i=0 ; i<DbRecordType::EoC; i++ )
    {
        sql << DbRecordType::ColumnName[i] << " " << DbRecordType::ColumnMeta[i];
        if ( i != DbRecordType::EoC-1 )
        {
            sql << ", ";
        }
    }
    sql << ");";

    DbConnection::instance().exec(sql.str(), true);
}

template <typename DbRecordType >
void save( DbRecordType & record )
{
    std::map<std::string, std::string> rawdata;
    record.save(rawdata);

    std::stringstream sql;
    sql << "insert into " << DbRecordType::TableName << " values(";
    for ( int i=0 ; i<DbRecordType::EoC; i++ )
    {
        if( rawdata.find(DbRecordType::ColumnName[i]) == rawdata.end() ) {
            sql << "NULL";
        } else {
            sql << "\"" << rawdata[DbRecordType::ColumnName[i]] << "\"";
        }
        if ( i != DbRecordType::EoC-1 )
        {
            sql << ", ";
        }
    }
    sql << ");";
    DbConnection::instance().exec(sql.str());
}

template <typename DbRecordType >
static bool find(const std::string & select,
                 const std::string & conditions,
                 const std::string & group,
                 const std::string & order,
                 const std::string & limit,
                 std::vector<DbRecordType> & records)
{
    std::stringstream sql;

    sql << "select " << (select.size() ? select : "*");
    sql << " from " << DbRecordType::TableName ;
    if ( conditions.size() )
    {
        sql << " where " << conditions;
    }
    if ( group.size() )
    {
        sql << " group by " << group;
    }
    if ( order.size() )
    {
        sql << " order by " << order;
    }
    if ( limit.size() )
    {
        sql << " limit " << limit;
    }
    sql << ";";
    std::cerr << sql.str() << std::endl;

    std::list< std::map<std::string, std::string> >sqlResults;
    if( DbRecordType::find_by_sql(sql.str(), sqlResults) == true ) {
        for (std::list< std::map<std::string, std::string> >::iterator it = sqlResults.begin();
            it!=sqlResults.end(); it++)
        {
            DbRecordType record;
            record.load(*it);
            records.push_back(record);
        }

        return true;
    }
    return false;
}

template <typename DbRecordType >
static bool del_record(const std::string & conditions)
{
    std::stringstream sql;

    sql << "delete ";
    sql << " from " << DbRecordType::TableName ;

    if(!conditions.empty())
    {
        sql << " where " << conditions;
    }
    sql << ";";
    std::cerr<< sql.str() << std::endl;

    if( DbRecordType::delete_by_sql(sql.str()) == true && DbConnection::instance().exec("vacuum") == true)
        return true;

    return false;
}



#define DEFINE_DB_RECORD_COMMON_ROUTINES(ClassName) \
    static void createTable() { \
        ::sf1r::createTable<ClassName>(); \
    } \
    \
    static bool find(const std::string & select, \
                     const std::string & conditions, \
                     const std::string & group, \
                     const std::string & order, \
                     const std::string & limit, \
                     std::vector<ClassName> & records) { \
        return ::sf1r::find(select, conditions, group, order, limit, records); \
    } \
    \
    void save() { \
        ::sf1r::save(*this); \
    }\
    \
    static bool del_record(const std::string & conditions) {\
        return ::sf1r::del_record<ClassName>(conditions); \
    }


}

#endif

