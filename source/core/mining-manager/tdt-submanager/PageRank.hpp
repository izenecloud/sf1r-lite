#ifndef SF1R_WIKI_PR_HPP_
#define SF1R_WIKI_PR_HPP_
#include <vector>
#include <set>
#include <string>
#include <iostream>
#include <math.h>
#include <icma/icma.h>
#include <icma/openccxx.h>
#include <am/succinct/wat_array/wat_array.hpp>
#include <am/succinct/wat_array/wavelet_matrix.hpp>
#include <am/succinct/fm-index/wavelet_matrix.hpp>
#include "TdtFactory.hpp"
using namespace std;
using namespace izenelib::am::succinct::fm_index;

namespace sf1r
{

// use graph store webpage, weight representlink times
struct CalText
{
    std::vector<int> linkin_;
    double pr_;
    double contentRelevancy_;
    int outNumber_;
    void PrintNode()
    {
        cout<<"pr"<<pr_<<"contentRelevancy"<<contentRelevancy_<<"out"<<outNumber_<<"in"<<linkin_.size()<<endl;
    }


};
class Node
{
public:
    Node(std::string name, int id=0);

    ~Node();

    //void InsertLinkInNode(int i) ;

    //double GetPageRank();

    // void SetPageRank(double pr);

    // void SetContentRelevancy(double contentRelevancy);

    void SetAdvertiRelevancy(double advertiRelevancy);

    // double CalcRank(const vector<Node*>& vecNode,const CalText& linkinSub);

    // size_t GetOutBoundNum() ;

    // size_t GetInBoundNum() ;

    //double GetContentRelevancy() ;

    double GetAdvertiRelevancy() ;

    std::string GetName() ;

    //void InsertLinkOutNode(int i) ;

    void PrintNode();

    void SetId(int id);

    int GetId();

    friend std::istream& operator>> ( std::istream &f, Node &node );
    friend std::ostream& operator<< ( std::ostream &f, const Node &node );
    friend void simplify(Node &node, cma::OpenCC* opencc);
    //set<int> linkin_index_;
  // vector<int> linkin_index_;
  // vector<int> linkout_index_;
   int offStart;
   int offStop;
  // int outNumber_;
  // int outNumberOrg_;
private:
    int id_;
    double advertiRelevancy_;

};

std::istream& operator>>(std::istream &f, vector<uint64_t>& A);

class PageRank
{
public:
    //explicit PageRank(std::vector<Node*>& nodes,std::set<int>& SubGraph,wat_array::WatArray& wa,double alpha=0.7,double beta=0.0);
    // explicit PageRank(std::vector<Node*>& nodes,std::set<int>& SubGraph, wavelet_matrix::WaveletMatrix& wa,double alpha=0.7,double beta=0.0);
    //explicit PageRank(std::vector<Node*>& nodes,std::set<int>& SubGraph,  WaveletMatrix<uint64_t>& wa,double alpha=0.7,double beta=0.0);
    explicit PageRank(std::vector<Node*>& nodes,std::set<int>& SubGraph,  TdtMemory* wa,double alpha=0.7,double beta=0.0);
    ~PageRank(void);
    void CalcAll(int n);
    double Calc(int index);
    void PrintPageRank(std::vector<Node*> & nodes);
    void InitMap();
    CalText& GetLinkin(int index);
    double CalcRank(const CalText& linkinSub);
    double GetPr(int index);
    void SetPr(int index,double pr);
    double GetContentRelevancy(int index);
    void SetContentRelevancy(int index,double contentRelevancy);
    double GetON(int index);
    void AddON(int index);

    std::set<int> &SubGraph_;
private:
    int GetIndexByOffset_(const int&  offSet);

    std::map<int,CalText> linkinMap_;
    std::vector<Node*> &nodes_;
    //wat_array::WatArray &wa_;
    //wavelet_matrix::WaveletMatrix  &wa_;
    //WaveletMatrix<uint64_t>  &wa_;
    TdtMemory* wa_;
    double alpha_; //内容阻尼系数
    double beta_; //广告阻尼系数
    bool InCludeList_;
};


}
#endif
