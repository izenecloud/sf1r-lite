#ifndef SF1R_WIKI_PR_HPP_
#define SF1R_WIKI_PR_HPP_
#include <vector>
#include <set>
#include <string>
#include <iostream>
#include <math.h>
#include <icma/icma.h>
#include <icma/openccxx.h>
using namespace std;

// use graph store webpage, weight representlink times
struct CalText
{
    vector<int> linkin_;
    double pr_;
    double contentRelevancy_;
    int outNumber_;
};
class Node 
{
public:
    Node(string name, int id=0);

    ~Node();
   
    void InsertLinkInNode(int i) ;

    //double GetPageRank();

   // void SetPageRank(double pr);

   // void SetContentRelevancy(double contentRelevancy);

    void SetAdvertiRelevancy(double advertiRelevancy);

   // double CalcRank(const vector<Node*>& vecNode,const CalText& linkinSub);

    size_t GetOutBoundNum() ;
 
    size_t GetInBoundNum() ;
   
    //double GetContentRelevancy() ;

    double GetAdvertiRelevancy() ;
   
    string GetName() ;
    
    void InsertLinkOutNode(int i) ;
   
    void PrintNode();
   
    void SetId(int id);
 
    int GetId();
    
   friend istream& operator>> ( istream &f, Node &node );
   friend ostream& operator<< ( ostream &f, const Node &node );
   friend void simplify(Node &node, cma::OpenCC* opencc);
   //set<int> linkin_index_;
   vector<int> linkin_index_;
   vector<int> linkout_index_;
   int outNumber_;
  // int outNumberOrg_;
private:
    string name_;
  //  double page_rank_;
    int id_;


    //set<Node*> linkin_nodes_;
    //set<Node*> linkout_nodes_;

    double contentRelevancy_;
    double advertiRelevancy_;
};

class PageRank
{
public:
    PageRank(vector<Node*>& nodes,set<int>& SubGraph,double alpha=0.7,double beta=0.15);
    ~PageRank(void);
    void CalcAll(int n);
    double Calc(int index);
    void PrintPageRank(vector<Node*> & nodes);
    CalText& getLinkin(int index);
    double CalcRank(const CalText& linkinSub);
    double getON(int index);
    double getPr(int index);
    double setContentRelevancy(int index,double contentRelevancy);
    double getContentRelevancy(int index);
    void InitMap();
    void setPr(int index,double pr);
    //void setVec(vector<Node*>& nodes)
    //{nodes_=&nodes;}
    set<int> &SubGraph_;
private:
    map<int,CalText>  linkinMap_;
    vector<Node*> &nodes_;

    double alpha_; //内容阻尼系数
    double beta_; //广告阻尼系数
};
#endif
