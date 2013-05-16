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
    UserQuery::find(
        "distinct query",
        "collection = '" + collectionName + "'" + " and " + "hit_docs_num > 0",
        "",
        "TimeStamp desc",
        boost::lexical_cast<std::string>(limit),
        query_records);

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


void LogAnalysis::getRecentKeywordFreqList(const std::string& collectionName, const std::string& time_string, std::vector<UserQuery>& queryList)
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
//        UserQuery::getRecentKeyword(time_string, res);
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

}
