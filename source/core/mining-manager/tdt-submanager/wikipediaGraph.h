#ifndef SF1R_WIKI_GRAPH_HPP_
#define SF1R_WIKI_GRAPH_HPP_
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include "PageRank.h"
#include "adBias.h"
#include "conBias.h"
//#include "database.h"
#include <algorithm>
#include <icma/icma.h>
#include <icma/openccxx.h>
using namespace std;
using namespace sf1r;




class wikipediaGraph
{

     
struct NodeCmp {
    bool operator() (Node* node1,Node* node2)
    {
        return node1->GetId() > node2->GetId();
    }

} NodeCmpOperator;
     vector<Node*> nodes_;

     ConBias contentBias;
     AdBias advertiseBias;
     //database db_;
     map<string,int> title2id;
     // PageRank pr_;
     //set<int> SubGraph_;
     string path_;
     string redirpath_;

public:
    cma::OpenCC* opencc_;
    
    wikipediaGraph(const string& cma_path,const string& wiki_path, cma::OpenCC* opencc);
    
    ~wikipediaGraph();
   
    void load(std::istream& is);
    
    void save(std::ostream& os);
  
    void initFromDb();
    
    void init();
   
    void flush();
    
    void InitSubGaph(const int& index,set<int>& SubGraph,int itertime=1);
  //  void InitGraph();
   
   
    void link2nodes(const int& i,const int& j);
    
    void GetTopics(const std::string& content, std::vector<std::string>& topic_list, size_t limit);
    
    void test();
   
    std::vector<pair<double,string> > SetContentBias(const string& content,PageRank& pr);
   
   // void pairBias(pair<string,uint32_t>& ReLativepair,bool reset=false);
   
    Node* getNode(const string&  str,bool& HaveGet);

    Node* getNode(const int&  id,bool& HaveGet);
 
    int  getIndex(const int&  id);
    
    void SetAdvertiseAll();
   
    void SetAdvertiseBias(Node* node);
 
    int Title2Id(const string& title);
   
    void CalPageRank(PageRank& pr);
   
    void BuildMap();
        
    void simplifyTitle();
 
    void  loadAll(std::istream &f );

    void  load(std::istream &f, int& id,string& name );

    void  InitOutLink();
 
   string ToSimplified(const string& name);
 
};



#endif
