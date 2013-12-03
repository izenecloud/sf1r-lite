#include "LogManager.h"
#include "LogAnalysis.h"
#include <query-manager/QMCommonFunc.h>

namespace sf1r
{

void LogAnalysis::getRecentKeywordList(const std::string& collectionName, uint32_t limit, std::vector<izenelib::util::UString>& recentKeywords)
{
    recentKeywords.resize(0);
    if (limit==0 ) return;
    std::vector<UserQuery> query_records;
    if(RDbConnection::instance().logServer())
    {
        std::list<std::map<std::string, std::string> > res;
        boost::posix_time::ptime time_now = boost::posix_time::microsec_clock::local_time();
        boost::posix_time::ptime p = time_now - boost::gregorian::days(20);
        std::string time_string = boost::posix_time::to_iso_string(p);

        UserQuery::getRecentKeyword(collectionName, time_string , res);
        std::list<std::map<std::string, std::string> >::iterator it;
        for(it = res.begin();it!=res.end();it++)
        {
            UserQuery uquery;
            uquery.load(*it);
            query_records.push_back(uquery);
        }
        query_records.resize(limit);
    }
    else
    {
        UserQuery::find(
            "distinct query",
            "collection = '" + collectionName + "'" + " and " + "hit_docs_num > 0",
            "",
            "TimeStamp desc",
            boost::lexical_cast<std::string>(limit),
            query_records);
    }

    std::vector<UserQuery>::const_iterator it = query_records.begin();
    for ( ; it!=query_records.end(); ++it )
    {
        izenelib::util::UString uquery(it->getQuery(), izenelib::util::UString::UTF_8);
        if ( QueryUtility::isRestrictWord( uquery ) )
        {
            continue;
        }
        recentKeywords.push_back( uquery );
    }
}


void LogAnalysis::getRecentKeywordFreqList(const std::string& collectionName, const std::string& time_string, std::vector<UserQuery>& queryList,bool fromautofill)
{
    if(RDbConnection::instance().logServer())
    {
        std::list<std::map<std::string, std::string> > res;
        UserQuery::getRecentKeyword(collectionName, time_string, res);
        std::list<std::map<std::string, std::string> >::iterator it;
        for(it=res.begin();it!=res.end();it++)
        {
            UserQuery uquery;
            uquery.load(*it);
            queryList.push_back(uquery);
        }
    }
    else
    {
        std::stringstream sql;
        if(!fromautofill)
        {
            sql << "select query ,max(hit_docs_num) as hit_docs_num,count(*) as count ";
            sql << " from " << UserQuery::TableName ;
            sql << " where " << ("collection = '" + collectionName + "' and hit_docs_num > 0 and TimeStamp >= '" + time_string +"'");
            sql << " group by " << "query";
            sql <<";";
        }
        else
        {
           sql << "  select t1.query,t1.hit_docs_num,t2.count ";
           sql << "  from " << UserQuery::TableName ;
           sql << "  t1 inner join ";
           sql << "  (select query,max(TimeStamp) as TimeStamp,count(*) as count  ";
           sql << "  from   " << UserQuery::TableName ;
           sql << "  where " << ("collection = '" + collectionName+ "' and TimeStamp >= '" + time_string +"' group by query) t2");
           sql << "  on t2.TimeStamp= t1.TimeStamp and  t2.query= t1.query ";
           sql << ";";
        }
        std::list<std::map<std::string, std::string> > res;
        UserQuery::find_by_sql(sql.str(), res);
        cout<<sql.str()<<endl;
        std::list<std::map<std::string, std::string> >::iterator it;
        for(it=res.begin();it!=res.end();it++)
        {
            UserQuery uquery;
            uquery.load(*it);
            queryList.push_back(uquery);
        }
    }
}

void LogAnalysis::getRecentKeywordFreqList(const std::string& collectionName, const std::string& time_string, boost::unordered_map<std::string, UserQuery> queryList)
{
    if(RDbConnection::instance().logServer())
    {
        std::list<std::map<std::string, std::string> > res;
        UserQuery::getRecentKeyword(collectionName, time_string, res);
        std::list<std::map<std::string, std::string> >::iterator it;
        for(it=res.begin();it!=res.end();it++)
        {
            UserQuery uquery;
            uquery.load(*it);
            std::string query = uquery.getQuery();
            if(queryList.find(query) == queryList.end())
            {
                queryList[query] = uquery;
            }
            else
            {
                queryList[query].setCount(queryList[query].getCount() + uquery.getCount());
                if(queryList[query].getHitDocsNum() < uquery.getHitDocsNum())
                    queryList[query].setHitDocsNum(uquery.getHitDocsNum());
            }
        }
    }
    else
    {
        std::stringstream sql;
        sql << "select query ,max(hit_docs_num) as hit_docs_num,count(*) as count ";
        sql << " from " << UserQuery::TableName ;
        sql << " where " << ("collection = '" + collectionName + "' and hit_docs_num > 0 and TimeStamp >= '" + time_string +"'");
        sql << " group by " << "query";
        sql <<";";
        std::list<std::map<std::string, std::string> > res;
        UserQuery::find_by_sql(sql.str(), res);
        std::list<std::map<std::string, std::string> >::iterator it;
        for(it=res.begin();it!=res.end();it++)
        {
            UserQuery uquery;
            uquery.load(*it);
            std::string query = uquery.getQuery();
            if(queryList.find(query) == queryList.end())
            {
                queryList[query] = uquery;
            }
            else
            {
                queryList[query].setCount(queryList[query].getCount() + uquery.getCount());
                if(queryList[query].getHitDocsNum() < uquery.getHitDocsNum())
                    queryList[query].setHitDocsNum(uquery.getHitDocsNum());
            }
        }
    }
}

void LogAnalysis::getRecentKeywordFreqList(const std::string& time_string, std::vector<UserQuery>& queryList)
{
    if(RDbConnection::instance().logServer())
    {
        std::list<std::map<std::string, std::string> > res;
        UserQuery::getRecentKeyword(time_string, res);
        std::list<std::map<std::string, std::string> >::iterator it;
        for(it=res.begin();it!=res.end();it++)
        {
            UserQuery uquery;
            uquery.load(*it);
            queryList.push_back(uquery);
        }
    }
    else
    {
        std::stringstream sql;
        sql << "select query ,max(hit_docs_num) as hit_docs_num,count(*) as count ";
        sql << " from "<<UserQuery::TableName;
        sql << " where " << (" hit_docs_num > 0 and TimeStamp >= '" + time_string +"'");
        sql << " group by query ;";
        std::list<std::map<std::string, std::string> > res;
        UserQuery::find_by_sql(sql.str(), res);
        std::list<std::map<std::string, std::string> >::iterator it;
        for(it=res.begin();it!=res.end();it++)
        {
            UserQuery uquery;
            uquery.load(*it);
            queryList.push_back(uquery);
        }
    }
}

void LogAnalysis::getPropertyLabel(const std::string& collection, std::vector<PropertyLabel>& propertyList)
{
    if(RDbConnection::instance().logServer())
    {
        std::list<std::map<std::string, std::string> > res;
        PropertyLabel::get_from_logserver(collection, res);
        std::list<std::map<std::string, std::string> >::iterator it;
        for(it=res.begin(); it!=res.end();it++)
        {
            PropertyLabel pl;
            pl.load(*it);
            propertyList.push_back(pl);
        }
    }
    else
    {
        std::stringstream sql;
        sql << "select label_name, sum(hit_docs_num) AS hit_docs_num";
        sql << " where collection = '" << collection <<"'";
        sql << " group by label_name;";
        std::list<std::map<std::string, std::string> > res;
        PropertyLabel::find_by_sql(sql.str(), res);
        std::list<std::map<std::string, std::string> >::iterator it;
        for(it=res.begin(); it!=res.end();it++)
        {
            PropertyLabel pl;
            pl.load(*it);
            propertyList.push_back(pl);
        }
    }
}

void LogAnalysis::delPropertyLabel(const std::string& collection)
{
    if(RDbConnection::instance().logServer())
    {
        PropertyLabel::del_from_logserver(collection);
    }
    else
    {
        std::stringstream sql;
        sql << "delete from " << PropertyLabel::TableName;
        sql << " where collection = '" << collection <<"';";
        PropertyLabel::delete_by_sql(sql.str()) && RDbConnection::instance().exec("vacuum");
    }
}

}
