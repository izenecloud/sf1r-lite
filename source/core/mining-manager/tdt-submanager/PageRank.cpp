#include "PageRank.hpp"
#include "WikiGraph.hpp"

#include <boost/thread/shared_mutex.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <iostream>
#include <sstream>
#include <algorithm>

using namespace std;
using namespace izenelib::am::succinct::fm_index;

namespace sf1r
{

std::istream& operator>>(std::istream &f, vector<uint64_t>& A)
{
      size_t len;
      f.read(( char*)&len, sizeof(len));
      int size=len;
      int index;
      //node.linkin_index_.resize(size);
      for(int i=0;i<size;i++)
      {
        f.read(( char*)&index, sizeof(index));
        A.push_back(index);
      }
 
      return f;
}

std::ostream& operator<<(std::ostream &f, const Node &node)
{
    //ReadLock lock(mutex_);
    f.write((const char*)&node.id_, sizeof(node.id_));
    int outNumber_;
    f.write((const char*)&outNumber_, sizeof(outNumber_));
    std::string name;
    name = WikiGraph::GetInstance()->GetTitleById(node.id_);
    size_t len=sizeof(name[0])*(name.size());
   // cout<<name<<endl;
    f.write((const char*)&len, sizeof(len));
    f.write((const char*)&name[0], sizeof(name[0])*(name.size()) );
/*
    len=node.linkin_index_.size();
    f.write((const char*)&len, sizeof(len));
    vector<int>::const_iterator citr = node.linkin_index_.begin();
    for (; citr !=node.linkin_index_.end(); ++citr)
    {
        int  index=(*citr);
        //cout<<"citr"<<index<<endl;
        f.write((const char*)&index, sizeof(index));
    }
*/
    return f;

}

std::istream& operator>>(std::istream &f, Node &node )
{
    size_t len;
// WriteLock lock(mutex_);


    f.read(( char*)&node.id_, sizeof(node.id_));
    int outNumber_;
    f.read(( char*)&outNumber_, sizeof(outNumber_));

    //size_t len=sizeof(name);
    f.read(( char*)&len, sizeof(len));
    std::string name;
    name.resize(len);
    f.read(( char*)&name[0], len);
    WikiGraph::GetInstance()->AddTitleIdRelation(name, node.id_);
   /*
    //string namestr(name,len);
    //node.name_=namestr;
    // cout<<node.name_<<endl;
    f.read(( char*)&len, sizeof(len));
    int size=len;
    //cout<<"len"<<len<<endl;
    int index;
// node.linkin_index_.reserve(size);
    node.linkin_index_.resize(size);
    for(int i=0; i<size; i++)
    {

        // cout<<"i"<<i<<endl;
        f.read(( char*)&index, sizeof(index));
        // cout<<"das"<<endl;
        node.linkin_index_[i]=index;
        //cout<<index<<endl;
        // node.linkin_index_.insert(index);
    }
   */
    return f;

}
void simplify(Node &node, cma::OpenCC* opencc)
{
/*
    std::string lowercase_content = node.name_;
    boost::to_lower(lowercase_content);
    std::string simplified_content;
    long ret = opencc->convert(lowercase_content, simplified_content);
    if(-1 == ret) simplified_content = lowercase_content;
    //if(node.name_!=simplified_content){cout<<node.name_<<simplified_content<<"endtag"<<endl;}
    node.name_=simplified_content;
*/
}

Node::Node(string name, int id)
    : id_(id)
    , advertiRelevancy_(0)
{
}

Node::~Node()
{
    //linkin_index_.clear();
}

void Node::SetAdvertiRelevancy(double advertiRelevancy)
{
    advertiRelevancy_ = log(log(advertiRelevancy+1.0)+1.0);
}

double Node::GetAdvertiRelevancy()
{
    return advertiRelevancy_;
}

string Node::GetName()
{
    return WikiGraph::GetInstance()->GetTitleById(id_);
}
/*
void Node::InsertLinkOutNode(int i)
{
    linkout_index_.push_back(i);
    outNumber_++;
}
*/
void Node::PrintNode()
{

    cout << "Node:" <<GetName() <<"advertiRelevancy"<<advertiRelevancy_<<"id"<<id_;//<<"outNumber_"<<outNumber_<<"inNumber"<< linkin_index_.size()<<endl;
}
void Node::SetId(int id)
{
    id_=id;
}

int Node::GetId()
{
    return id_;
}

//PageRank::PageRank(vector<Node*>& nodes,set<int>& SubGraph, wat_array::WatArray& wa,double alpha,double beta)
//PageRank::PageRank(vector<Node*>& nodes,set<int>& SubGraph,wavelet_matrix::WaveletMatrix& wa,double alpha,double beta)
//PageRank::PageRank(vector<Node*>& nodes,set<int>& SubGraph, WaveletMatrix<uint64_t>& wa,double alpha,double beta)
PageRank::PageRank(std::vector<Node*>& nodes,std::set<int>& SubGraph,  WavletTree* wa,double alpha,double beta)
    : SubGraph_(SubGraph)
    , nodes_(nodes)
    , wa_(wa)
    , alpha_(alpha)
    , beta_(beta)
    , InCludeList_(false)
{
    // cout<<"PageRankBuild"<<endl;
    // q_ must < 1
}


PageRank::~PageRank()
{
}

void PageRank::InitMap()
{

    set<int>::const_iterator citr = SubGraph_.begin();
    boost::posix_time::ptime time_now = boost::posix_time::microsec_clock::local_time();
    wat_array::BitTrie SubTrie( nodes_.size());//wa_->alphabet_num()


    for (; citr != SubGraph_.end(); ++citr)
    {
           // cout<<"Link in"<<(*citr)<<endl;
         SubTrie.insert(*citr);
         //cout<<(*citr)<<"exsit"<<SubTrie.exist(*citr)<<endl;
    }
    citr = SubGraph_.begin();
    for (; citr != SubGraph_.end(); ++citr)
    {
           // cout<<"Link in"<<(*citr)<<endl;
           Node * node =nodes_[(*citr)];
           vector<int> linkin;
           vector<uint64_t> ret;
           //cout<<"QuantileRangeAll"<<endl;
          /*
           for(unsigned i=node->offStart;i<=node->offStop;i++)
           {
                int index=wa_->Lookup(i);
                if(SubGraph_.find(index)!=SubGraph_.end())
                {
                     linkin.push_back(index);
                     
                }
           }
                       */
           wa_->QuantileRangeAll(node->offStart, node->offStop, ret,SubTrie);
           //cout<<"ret size"<<ret.size()<<endl;
           
           for(unsigned i=0;i<ret.size();i++)
           {
               int index=ret[i];
               //if(SubGraph_.find(index)!=SubGraph_.end())
               //{
                     linkin.push_back(index);
                     
               //}
               
           }

           //cout<<"linkin size"<<linkin.size()<<endl;
         
           CalText temp;
           temp.linkin_=linkin;
           temp.pr_=1.0;
           temp.contentRelevancy_=0.0;
           temp.outNumber_=0;
           linkinMap_.insert(pair<int,CalText>((*citr), temp));
           //node->PrintNode();
           //cout<<"innumber"<<iN<<endl;
    }
    citr = SubGraph_.begin();
    int RelativeNum=0;
    for (; citr != SubGraph_.end(); ++citr)
    {

           vector<int>::const_iterator sitr =GetLinkin(*citr).linkin_.begin();
           for (; sitr !=GetLinkin(*citr).linkin_.end(); ++sitr)
           {
                AddON(*sitr);
           }
           if(GetLinkin(*citr).linkin_.size()>100)
           {RelativeNum++;}
    }
    if(RelativeNum>50){InCludeList_=true;}
    time_now = boost::posix_time::microsec_clock::local_time();
}
// 迭代计算n次
void PageRank::CalcAll(int n)
{

    set<int>::const_iterator citr;

    for (int i=0; i<n; ++i)
    {

        citr = SubGraph_.begin();
        // cout<<"pr"<<pr<<endl;
        for (; citr != SubGraph_.end(); ++citr)
        {
            // cout<<"Link in"<<(*citr)<<endl;

           // node->PrintNode();
           // cout<<"pr"<<pr<<endl;
            Calc(*citr);
        }
        /**/

    }


    /*
     for (; citr != SubGraph_.end(); ++citr)
    {
           // cout<<"Link in"<<(*citr)<<endl;
            Node * node =nodes_[(*citr)];
            node->outNumber_=node->outNumberOrg_;

    }
    */
    //linkinMap_.clear();

}

void PageRank::PrintPageRank(vector<Node*> & nodes)
{
    //double total_pr = 0;
    vector<Node*>::const_iterator citr = nodes.begin();
    for (; citr!=nodes.end(); ++citr)
    {
        //Node * node = *citr;
        //node->PrintNode();
        //total_pr += node->GetPageRank();
    }
    //cout << "Total PR:" << total_pr << endl;
}

double PageRank::Calc(int index)
{
    // cout<<"CalcRank(nodes_)"<<endl;
    //cout<<"nodes size()"<<nodes_.size()<<endl;
    Node * node =nodes_[index];
    CalText &temp=GetLinkin(index);
    double pr = CalcRank(temp);
    // cout<<"pr"<<pr<<endl;
    if (pr < 0.00000000000000000000001 && pr > -0.00000000000000000000001) //pr == 0
    {
        pr = alpha_*temp.contentRelevancy_+beta_*node->GetAdvertiRelevancy() ;
    }
    else
    {
        // (1-alpha_-beta_)
        pr = pr*(1-alpha_-beta_)+ alpha_*temp.contentRelevancy_+beta_*node->GetAdvertiRelevancy();
    }
    // cout<<"setPr"<<endl;
    // node->SetPageRank(pr);
    temp.pr_=pr;
    return pr;
}

CalText& PageRank::GetLinkin(int index)
{
    map<int,CalText >::iterator it;
    it = linkinMap_.find(index);

    if(it !=linkinMap_.end())
    {
        return (it->second);
    }
    else
    {
        //cout<<"error GetLinkin"<<endl;
        static CalText null;
        return null;
    }
}

double PageRank::GetPr(int index)
{
    //cout<<"GetPr"<<index<<endl;
    return GetLinkin(index).pr_;
}

void PageRank::SetPr(int index,double pr)
{
    // cout<<"SetPr"<<index<<endl;
    GetLinkin(index).pr_=pr;
}

void PageRank::SetContentRelevancy(int index,double contentRelevancy)
{
    // cout<<"setContentRelevancy"<<index<<endl;
    if(nodes_[index]->GetName()=="upload_log")
        return;
    if(GetLinkin(index).contentRelevancy_>0.0&&contentRelevancy<1.0)
    {
      if(GetLinkin(index).contentRelevancy_<0.4)
        GetLinkin(index).contentRelevancy_+=0.3;
      else
        GetLinkin(index).contentRelevancy_=0.7;
    }
    else
    {
       
       if(contentRelevancy<1.0&&((!InCludeList_)||GetLinkin(index).linkin_.size()<100))
        GetLinkin(index).contentRelevancy_=min(contentRelevancy*contentRelevancy*GetLinkin(index).linkin_.size()*GetLinkin(index).outNumber_,0.20)+0.01;
       else
        GetLinkin(index).contentRelevancy_=contentRelevancy;
    }
    if(contentRelevancy>=1)
    {
        //CalText &linkinSub=GetLinkin(index);
        
        int size=wa_->Freq(index) ; 
        for (int i=0; i<size ; i++)
        {
             SetContentRelevancy(GetIndexByOffset_(wa_->Select(index,i)),0.01);
        }
        /*
        set<int>::const_iterator citr = SubGraph_.begin();
        for (; citr != SubGraph_.end(); ++citr)
        {
           // cout<<"Link in"<<(*citr)<<endl;

           vector<int> vectemp=GetLinkin(*citr).linkin_;
           if (find(vectemp.begin(), vectemp.end(),index )!=vectemp.end())
           {
              SetContentRelevancy((*citr),0.01);
           }

         
        }
        */
    }
}

int PageRank::GetIndexByOffset_(const int&  offSet)
{
       
       int front=0;
       int end=nodes_.size()-1;
       int mid=(front+end)/2;
       while(front<end && (nodes_[mid]->offStart>offSet||nodes_[mid]->offStop<offSet))
       {
          if(nodes_[mid]->offStop<offSet)front=mid+1;
          if(nodes_[mid]->offStart>offSet)end=mid-1;
          mid=front + (end - front)/2;
       }
      // cout<<"mid"<<mid<<endl;
       if(nodes_[mid]->offStart>offSet||nodes_[mid]->offStop<offSet)
       {
         // HaveGet=false;
          cout<<"offset false"<<endl;
          return  -1;
       }
       else
       {
         // HaveGet=true;
          return mid;
       }
};

double PageRank::GetContentRelevancy(int index)
{
    return GetLinkin(index).contentRelevancy_;
}

double PageRank::GetON(int index)
{
    // cout<<"GetON"<<index<<endl;
    return double(GetLinkin(index).outNumber_);
}

void PageRank::AddON(int index)
{
       // cout<<"GetON"<<index<<endl;
          
     GetLinkin(index).outNumber_++;
            
}

double PageRank::CalcRank(const CalText& linkinSub)
{
    double pr = 0;

    vector<int>::const_iterator citr = linkinSub.linkin_.begin();
    // cout<<"pr"<<pr<<endl;
    for (; citr != linkinSub.linkin_.end(); ++citr)
    {
        // cout<<"Link in"<<(*citr)<<endl;
        // Node * node = nodes_[(*citr)];
        //node->PrintNode();
        pr += GetPr(*citr)/GetON(*citr);
        // cout<<"pr"<<pr<<endl;
    }

    return pr;
}

}
