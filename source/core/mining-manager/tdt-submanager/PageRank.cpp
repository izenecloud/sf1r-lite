#include "PageRank.hpp"

#include <boost/thread/shared_mutex.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <iostream>
#include <sstream>
#include <algorithm>

using namespace std;

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
    size_t len=sizeof(node.name_[0])*(node.name_.size());
   // cout<<node.name_<<endl;
    f.write((const char*)&len, sizeof(len));
    f.write((const char*)&node.name_[0], sizeof(node.name_[0])*(node.name_.size()) );
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
    return f;
*/
}

std::istream& operator>>(std::istream &f, Node &node )
{
    size_t len;
// WriteLock lock(mutex_);


    f.read(( char*)&node.id_, sizeof(node.id_));
    int outNumber_;
    f.read(( char*)&outNumber_, sizeof(outNumber_));

    //size_t len=sizeof(node.name_);
    f.read(( char*)&len, sizeof(len));

    node.name_.resize(len);
    f.read(( char*)&node.name_[0], len);
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
    std::string lowercase_content = node.name_;
    boost::to_lower(lowercase_content);
    std::string simplified_content;
    long ret = opencc->convert(lowercase_content, simplified_content);
    if(-1 == ret) simplified_content = lowercase_content;
    //if(node.name_!=simplified_content){cout<<node.name_<<simplified_content<<"endtag"<<endl;}
    node.name_=simplified_content;
}

Node::Node(string name, int id)
    : name_(name)
    , id_(id)
    , advertiRelevancy_(0)
{
}

Node::~Node()
{
    //linkin_index_.clear();
}
/*
void InsertLinkdInNode(Node* node)
{
    //如果没有链接
    if (linkin_nodes_.find(node) == linkin_nodes_.end())
    {
        linkin_nodes_.insert(node);
    }
    node->InsertLinkOutNode(this);
}

void InsertLinkOutNode(Node* node)
{
    //如果没有链接
    if (linkout_nodes_.find(node) == linkout_nodes_.end())
    {
        linkout_nodes_.insert(node);
    }
}


void Node::InsertLinkInNode(int i)
{
    //如果没有链接top
    linkin_index_.push_back(i);
}


    double Node::GetPageRank()
    {
        return page_rank_;
    }

    void Node::SetPageRank(double pr)
    {
        page_rank_ = pr;
    }

    void Node::SetContentRelevancy(double contentRelevancy)
    {
        contentRelevancy_ = contentRelevancy;
    }
*/
void Node::SetAdvertiRelevancy(double advertiRelevancy)
{
    advertiRelevancy_ = log(log(advertiRelevancy+1.0)+1.0);
}
/*
    double Node::CalcRank(const vector<Node*>& vecNode,const CalText& linkinSub)
    {
        double pr = 0;

        vector<int>::const_iterator citr = linkin_index_.begin();
       // cout<<"pr"<<pr<<endl;
        for (; citr != linkin_index_.end(); ++citr)
        {
           // cout<<"Link in"<<(*citr)<<endl;
            Node * node = vecNode[(*citr)];
            //node->PrintNode();
           // if(node->GetPageRank()>0.0)
            pr += node->GetPageRank()/double(node->GetOutBoundNum());
           // cout<<"pr"<<pr<<endl;
        }


        vector<int>::const_iterator citr = linkinSub.linkin_.begin();
       // cout<<"pr"<<pr<<endl;
        for (; citr != linkinSub.linkin_.end(); ++citr)
        {
           // cout<<"Link in"<<(*citr)<<endl;
            Node * node = vecNode[(*citr)];
            //node->PrintNode();
            pr += node->GetPageRank()/double(node->GetOutBoundNum());
           // cout<<"pr"<<pr<<endl;
        }

        return pr;
    }

size_t Node::GetOutBoundNum()
{
    return  outNumber_;
}

size_t Node::GetInBoundNum()
{
    return  linkin_index_.size();
}


    double Node::GetContentRelevancy()
    {
        return contentRelevancy_;
    }
*/

double Node::GetAdvertiRelevancy()
{
    return advertiRelevancy_;
}

string Node::GetName()
{
    return name_;
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

    cout << "Node:" <<name_ <<"advertiRelevancy"<<advertiRelevancy_<<"id"<<id_;//<<"outNumber_"<<outNumber_<<"inNumber"<< linkin_index_.size()<<endl;
}
void Node::SetId(int id)
{
    id_=id;
}

int Node::GetId()
{
    return id_;
}

PageRank::PageRank(vector<Node*>& nodes,set<int>& SubGraph, wat_array::WatArray& wa,double alpha,double beta)
    : SubGraph_(SubGraph)
    , nodes_(nodes)
    , wa_(wa)
    , alpha_(alpha)
    , beta_(beta)
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
    cout<<"Init"<<time_now<<endl;
    wat_array::BitTrie SubTrie( nodes_.size());//wa_.alphabet_num()
    //SubTrie.insert(1312342);
    //cout<<"1312342 exsit"<<SubTrie.exist(1312342)<<endl;
    //cout<<"1312331 exsit"<<SubTrie.exist(1312331)<<endl;
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
          //  int oN=0,iN=0;
          /*
            vector<int>::const_iterator sitr = node->linkout_index_.begin();
            vector<int> linkin;
         /*
           for (; sitr !=node->linkout_index_.end(); ++sitr)
           {
                if(SubGraph_.find(*sitr)!=SubGraph_.end())
                {oN++;}
           
           }
         
           sitr = node->linkin_index_.begin();
           
           for (; sitr !=node->linkin_index_.end(); ++sitr)
           {
                if(SubGraph_.find(*sitr)!=SubGraph_.end())
                {
                     linkin.push_back(*sitr);
                     
                }
           
           }
           */
           vector<uint64_t> ret;
           //cout<<"QuantileRangeAll"<<endl;
           wa_.QuantileRangeAll(node->offStart, node->offStop, ret,SubTrie);
           //cout<<"ret size"<<ret.size()<<endl;
           for(unsigned i=0;i<ret.size();i++)
           {
                int index=ret[i];
                if(SubGraph_.find(index)!=SubGraph_.end())
                {
                     linkin.push_back(index);
                     
                }
           }
          // cout<<"linkin size"<<linkin.size()<<endl;
           /*
           for(unsigned i=node->offStart;i<=node->offStop;i++)
           {
                int index=wa_.Lookup(i);
                if(SubGraph_.find(index)!=SubGraph_.end())
                {
                     linkin.push_back(index);
                     
                }
           }
           */
           CalText temp;
           temp.linkin_=linkin;
           temp.pr_=1.0;
           temp.contentRelevancy_=0.0;
           temp.outNumber_=0;
           linkinMap_.insert(pair<int,CalText>((*citr), temp));
           //node->PrintNode();
           //cout<<"innumber"<<iN<<endl;
    }
    time_now = boost::posix_time::microsec_clock::local_time();
    cout<<"End"<<time_now<<endl;
    citr = SubGraph_.begin();
    for (; citr != SubGraph_.end(); ++citr)
    {
           // cout<<"Link in"<<(*citr)<<endl;

           vector<int>::const_iterator sitr =getLinkin(*citr).linkin_.begin();
           for (; sitr !=getLinkin(*citr).linkin_.end(); ++sitr)
           {
                addON(*sitr);
           }

         
    }
}
// 迭代计算n次
void PageRank::CalcAll(int n)
{

    set<int>::const_iterator citr;
    boost::posix_time::ptime time_now = boost::posix_time::microsec_clock::local_time();
    // cout<<"subset"<<time_now<<endl;
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
    time_now = boost::posix_time::microsec_clock::local_time();
    // cout<<"done"<<time_now<<endl;
}

void PageRank::PrintPageRank(vector<Node*> & nodes)
{
    //double total_pr = 0;
    vector<Node*>::const_iterator citr = nodes.begin();
    for (; citr!=nodes.end(); ++citr)
    {
        Node * node = *citr;
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
    CalText &temp=getLinkin(index);
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

CalText& PageRank::getLinkin(int index)
{
    map<int,CalText >::iterator it;
    it = linkinMap_.find(index);

    if(it !=linkinMap_.end())
    {
        return (it->second);
    }
    else
    {
        //cout<<"error getLinkin"<<endl;
        static CalText null;
        return null;
    }
}

double PageRank::getPr(int index)
{
    //cout<<"getPr"<<index<<endl;
    return getLinkin(index).pr_;
}

void PageRank::setPr(int index,double pr)
{
    // cout<<"SetPr"<<index<<endl;
    getLinkin(index).pr_=pr;
}

void PageRank::setContentRelevancy(int index,double contentRelevancy)
{
    // cout<<"setContentRelevancy"<<index<<endl;
    if(nodes_[index]->GetName()=="upload_log")
        return;
    if(getLinkin(index).contentRelevancy_>0.0&&contentRelevancy<1.0)
    {
        getLinkin(index).contentRelevancy_+=0.3;
    }
    else
    {
        getLinkin(index).contentRelevancy_=contentRelevancy;
    }
    if(contentRelevancy>=1)
    {
        //CalText &linkinSub=getLinkin(index);
        int size=wa_.Freq(index) ; 
        for (int i=0; i<size ; i++)
        {
             setContentRelevancy(getIndexByOffset(wa_.Select(index,i)),0.01);
        }
                
    }
}
int  PageRank::getIndexByOffset(const int&  offSet)
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

double PageRank::getContentRelevancy(int index)
{
    return getLinkin(index).contentRelevancy_;
}

double PageRank::getON(int index)
{
    // cout<<"getON"<<index<<endl;
    return double(getLinkin(index).outNumber_);
}

void PageRank::addON(int index)
{
       // cout<<"getON"<<index<<endl;
          
     getLinkin(index).outNumber_++;
            
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
        pr += getPr(*citr)/getON(*citr);
        // cout<<"pr"<<pr<<endl;
    }

    return pr;
}

}
