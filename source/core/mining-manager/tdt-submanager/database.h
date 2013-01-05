#ifndef SF1R_WIKI_DB_HPP_
#define SF1R_WIKI_DB_HPP_
#include <stdlib.h>
#include <stdio.h>

#include <mysql/mysql.h>
#include <fstream>
#include <string.h>
#include <iostream>
#include <list>
#include <map>
#include <vector>
#include <boost/lexical_cast.hpp>   
//#include <boost/thread/mutex.hpp>
#include <list>
//#include <boost/filesystem.hpp>
using namespace std;

namespace sf1r
{

class database
{
MYSQL* wiki;


public:
map<string,int> title2id;
database()
{
    cout<<"DataBaseBuild"<<endl;
    wiki=createdb();
};
~database()
{
   mysql_close(wiki);
};
MYSQL* createdb(string dbname="title2id")
{
   MYSQL *conn_ptr;

   conn_ptr = mysql_init(NULL);
   if (!conn_ptr) {
      fprintf(stderr, "mysql_init failed\n");
      return NULL;
   }
   /*
    IP：180.153.140.119
    port: 3306
    DB name: b5m_category
    userName: data
    password: izene123
   */
   /* 
   mysql_options(conn_ptr, MYSQL_SET_CHARSET_NAME, "utf8");
   conn_ptr = mysql_real_connect(conn_ptr, "180.153.140.119", "data", "izene123",
                                                     "b5m_category", 3306, NULL, 0);//连接数据库
   */
   mysql_options(conn_ptr, MYSQL_SET_CHARSET_NAME, "utf8");
   conn_ptr = mysql_real_connect(conn_ptr, "127.0.0.1", "root", "root",
                                                     dbname.c_str(), 3306, NULL, 0);//连接数据库
   if (conn_ptr) {
      printf("Connection success\n");
   } else {
      printf("Connection failed\n");
   }

 //  mysql_close(conn_ptr);

   return conn_ptr;
} 
bool fetchRows_(MYSQL* db, std::list< std::map<std::string, std::string> > & rows)
{
    bool success = true;
    int status = 0;
    while (status == 0)
    {   
        MYSQL_RES* result = mysql_store_result(db);
        if (result)
        {    

            int num_cols = mysql_num_fields(result);
            std::vector<std::string> col_nums(num_cols);
            for (int i=0;i<num_cols;i++)
            {
                MYSQL_FIELD* field = mysql_fetch_field_direct(result, i);
                col_nums[i] = field->name;               
            }

            MYSQL_ROW row;
            while ((row = mysql_fetch_row(result)))
            { 
                std::map<std::string, std::string> map;
                for (int i=0;i<num_cols;i++)
                {   
                    if(!row[i])
                    { map.insert(std::make_pair(col_nums[i],"null") );}
                    else
                    {
                      map.insert(std::make_pair(col_nums[i], row[i]) );
                    }
                    
                }
                rows.push_back(map);
             }                
                mysql_free_result(result);    
        }
        else
        {
            if (mysql_field_count(db))
            {
                success = false;
                break;
            }
        }

        if ((status = mysql_next_result(db)) > 0)
        {
            success = false;
            break;
        }
    }

    return success;
}


bool exec(const std::string & sql,std::list< std::map<std::string, std::string> > & results,bool omitError)
{
    bool ret = true;
    MYSQL* db = wiki;
    if ( db == NULL )
    {
        cout << " No available connection";
        return false;
    }

    if ( mysql_real_query(db, sql.c_str(), sql.length()) >0 && mysql_errno(db) )
    {
        ret = false;
    }
    else
    {
        ret = fetchRows_(db, results);
    }
    return ret;
}
void initGraph(vector<Node*> & nodes)
{
        buildNodes(nodes);  
        wiki=createdb("wiki");
        for(int i=0;i<100;i++)
        buildLinks(nodes,i);
        buildLinks(nodes,100,true);
}
void buildNodes(vector<Node*> & nodes)
{
      std::list< std::map<std::string, std::string> > results;
      cout<<"results"<<endl;


      string sql="select * from page";
      exec(sql,results,true);
      cout<<"exec"<<endl;
      nodes.resize(results.size());
      cout<<"nodes"<<nodes.size()<<endl;
      //cout<<"ds5"<<endl;
      int i=0;
      if(!results.empty())
      {
         std::list< std::map<std::string, std::string> >::iterator iter=results.begin();
         while(iter!=results.end())
         {
            int page_id;
            string page_title;
            std::map<std::string, std::string> temp=(*iter);
            std::map<std::string, std::string>::iterator it= temp.find("page_id");
            if(it != temp.end()) 
            {
               page_id=boost::lexical_cast<int>(it->second);
            }
            it= temp.find("page_title");
            if(it != temp.end()) 
            {
               page_title=it->second;
              // return TODO;
            }
            title2id.insert(pair<string,int>(page_title,page_id));
            Node* tempNode=new Node( page_title,page_id);
            nodes[i]=tempNode;
            i++;
            iter++;

        }
      }
      else
      {
      }
   
  
 
}
void buildLinks(vector<Node*> & nodes,int tong,bool last=false)
{
      std::list< std::map<std::string, std::string> > results;
      string sql;
      if(!last)
      sql="select * from pagelinks where pl_from>"+boost::lexical_cast<string>(tong*100000)+" and pl_from<"+boost::lexical_cast<string>((tong+1)*100000);
      else
      sql="select * from pagelinks where pl_from>"+boost::lexical_cast<string>((tong+1)*100000);
      exec(sql,results,true);
      cout<<"LinkNum"<<results.size()<<endl;
      //cout<<"ds5"<<endl;
      if(!results.empty())
      {
         std::list< std::map<std::string, std::string> >::iterator iter=results.begin();
         while(iter!=results.end())
         {
            int plfrom,plid;
            string pltitle;
            std::map<std::string, std::string> temp=(*iter);
            std::map<std::string, std::string>::iterator it= temp.find("pl_from");
            if(it != temp.end()) 
            {
               plfrom=boost::lexical_cast<int>(it->second);
            }
            it= temp.find("pl_title");
            if(it != temp.end()) 
            {
               pltitle=it->second;
              // return TODO;
            }
            iter++;
            map<string, int>::iterator titleit;
            titleit = title2id.find(pltitle);

            if(titleit != title2id.end())
            {

                 plid=titleit->second;

            }

            link2nodes(id2index(plid,nodes),id2index(plfrom,nodes),nodes);
        }
      }
      else
      {
      }
   
  
 
}
inline void link2nodes(int i,int j,vector<Node*> & nodes)
{
    nodes[i]->InsertLinkInNode(j); 
    nodes[j]->InsertLinkOutNode();
}
inline int id2index(int id,vector<Node*> & nodes)
{
   
    int front=0;
    int end=nodes.size()-1;
    int mid=(front+end)/2;
    while(front<end && nodes[mid]->GetId()!=id)
    {
        if(nodes[mid]->GetId()<id)front=mid+1;
        if(nodes[mid]->GetId()>id)end=mid-1;
        mid=front + (end - front)/2;
    }
    if(nodes[mid]->GetId()!=id)
    {
        return -1;//(error)
    }
    else
    {
        return mid;
    }
}


};

}
#endif
