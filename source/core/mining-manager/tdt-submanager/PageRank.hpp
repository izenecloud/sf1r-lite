#ifndef SF1R_WIKI_PR_HPP_
#define SF1R_WIKI_PR_HPP_
#include <vector>
#include <set>
#include <string>
#include <iostream>
#include <math.h>
#include <icma/icma.h>
#include <icma/openccxx.h>

namespace sf1r
{

// use graph store webpage, weight representlink times
struct CalText
{
    std::vector<int> linkin_;
    double pr_;
    double contentRelevancy_;
    int outNumber_;
};
class Node
{
public:
    Node(std::string name, int id=0);

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

    std::string GetName() ;

    void InsertLinkOutNode(int i) ;

    void PrintNode();

    void SetId(int id);

    int GetId();

    friend std::istream& operator>> ( std::istream &f, Node &node );
    friend std::ostream& operator<< ( std::ostream &f, const Node &node );
    friend void simplify(Node &node, cma::OpenCC* opencc);
    //set<int> linkin_index_;
    std::vector<int> linkin_index_;
    std::vector<int> linkout_index_;
    int outNumber_;
    // int outNumberOrg_;
private:
    std::string name_;
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
    PageRank(std::vector<Node*>& nodes,std::set<int>& SubGraph,double alpha=0.7,double beta=0.15);
    ~PageRank(void);
    void CalcAll(int n);
    double Calc(int index);
    void PrintPageRank(std::vector<Node*> & nodes);
    CalText& getLinkin(int index);
    double CalcRank(const CalText& linkinSub);
    double getON(int index);
    double getPr(int index);
    void setContentRelevancy(int index,double contentRelevancy);
    double getContentRelevancy(int index);
    void InitMap();
    void setPr(int index,double pr);
    //void setVec(vector<Node*>& nodes)
    //{nodes_=&nodes;}
    std::set<int> &SubGraph_;
private:
    std::map<int,CalText>  linkinMap_;
    std::vector<Node*> &nodes_;

    double alpha_; //内容阻尼系数
    double beta_; //广告阻尼系数
};


}
#endif
