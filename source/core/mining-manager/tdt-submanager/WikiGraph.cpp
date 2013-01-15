#include "WikiGraph.hpp"
#include <math.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/bind.hpp>
#include <boost/thread/once.hpp>

namespace sf1r
{
WikiGraph::WikiGraph()
{
     WavletFactory factorytemp;
     enum WAVLETTYPE test=WAVLET_MATRIX;
     wa_=factorytemp.CreateWavletTree(test);//
     advertiseBias_=NULL;
}

WikiGraph::~WikiGraph()
{

    for(unsigned i=0; i<nodes_.size(); i++)
    {
        delete nodes_[i];
    }
    wa_->Clear();
    delete advertiseBias_;
    titleIdDbTable_->close();
    idTitleDbTable_->close(); 
    delete titleIdDbTable_;
    delete idTitleDbTable_;
}

void WikiGraph::Init(const string& wiki_path,cma::OpenCC* opencc)
{
    static boost::once_flag once = BOOST_ONCE_INIT;

    boost::call_once(once, boost::bind(&WikiGraph::SetParam_, this, wiki_path, opencc));
}

void WikiGraph::SetParam_(const string& wiki_path,cma::OpenCC* opencc)
   // : advertiseBias_((boost::filesystem::path(wiki_path)/=boost::filesystem::path("AdvertiseWord.txt")).c_str())
   // , opencc_(opencc)
{
    if(advertiseBias_){return;}

    advertiseBias_=new AdBias((boost::filesystem::path(wiki_path)/=boost::filesystem::path("AdvertiseWord.txt")).c_str()) ;

    opencc_=opencc;   
    boost::filesystem::path wikigraph_path(wiki_path);
    wikigraph_path /= boost::filesystem::path("wikigraph");
    path_ = wikigraph_path.c_str();

    boost::filesystem::path redirect_path(wiki_path);
    redirect_path /= boost::filesystem::path("redirect");
    redirpath_ = redirect_path.c_str();
    boost::filesystem::path stopword_path(wiki_path);
    stopword_path /= boost::filesystem::path("StopWord.txt");
    stopwordpath_ = stopword_path.c_str();
   
    InitStopword_();
    titleIdDbTable_ = new TitleIdDbTable(wiki_path+"/titleid");
    idTitleDbTable_ = new IdTitleDbTable(wiki_path+"/idtitle");
    titleIdDbTable_->open();
    idTitleDbTable_->open();

    LOG(INFO)<<"wikipediaGraphBuild"<<endl;
    //LOG(INFO)<<"init wiki_path"<<wiki_path<<" "<<path_<<endl;

    Init_();

    BuildMap_();
   
    SetAdvertiseAll_();
    //titleIdDbTable_->close();
    //idTitleDbTable_->close(); 
    LOG(INFO)<<"wikipediaGraphDone"<<endl;

}

void WikiGraph::Load_(std::istream& is)
{
        vector<uint64_t> A;
        unsigned int nodesize;
        is.read(( char*)&nodesize, sizeof(nodesize));
        nodes_.resize(nodesize);

        for(unsigned int i=0;i<nodesize;i++)
        { 
          //if(i%10000==0){LOG(INFO)<<"load "<<i<<"nodes"<<endl;}
          nodes_[i]=new Node("");
          is>>(*nodes_[i]);
          nodes_[i]->offStart  =A.size();
          is>>A;
          nodes_[i]->offStop  =A.size()-1;
        }
        
        wa_->Init(A);
        
}
void WikiGraph::Save_(std::ostream& os)
{
    unsigned int nodesize=nodes_.size();
    os.write(( char*)&nodesize, sizeof(nodesize));
    for(unsigned int i=0; i<nodes_.size(); i++)
    {
        os<<(*nodes_[i]);
    }
}

void WikiGraph::Load_(std::istream &f, int& id,string& name )
{
    f.read(( char*)&id, sizeof(id));
    size_t len;
    f.read(( char*)&len, sizeof(len));
    name.resize(len);
    f.read(( char*)&name[0], len);
}

void WikiGraph::LoadAll_(std::istream &f )
{
    unsigned size;
    f.read(( char*)&size, sizeof(size));
    for(unsigned i=0; i<size; i++)
    {
        int id;
        string name;
        Load_(f,id,name);
        // LOG(INFO)<<"name"<<name<<"id"<<id<<endl;
        if(stopword_.find(ToSimplified_(name))==stopword_.end())
        {
          redirect_.insert(pair<int,string>(id,ToSimplified_(name)));
          // LOG(INFO)<<id<<" "<<name<<endl;
        }
        // title2id.insert(pair<string,int>(ToSimplified(name),id));

        // if(i%10000==0){LOG(INFO)<<"have build map"<<nodes_.size()+i<<endl;}
    }
}

void WikiGraph::Init_()
{

    std::ifstream ifs(path_.c_str());
    if (ifs) Load_(ifs);
    ifs.close();
}

void WikiGraph::Flush_()
{
    if (path_.empty()) return;
    std::ofstream ofs("wikigraph");
    if (ofs) Save_(ofs);
    ofs.close();
}

void WikiGraph::GetTopics(const std::vector<std::pair<std::string,uint32_t> >& relativeWords, std::vector<std::string>& topic_list, size_t limit)
{
    //LOG(INFO)<<"SetContentBias";
   // LOG(INFO)<<"word segment end"<<endl;
    set<int> SubGraph;
    PageRank pr(nodes_,SubGraph,wa_);
    std::vector<pair<double,string> > NotInGraph;
    SetContentBias_(relativeWords,pr,NotInGraph);

    //pr_.PrintPageRank(nodes_);
    // LOG(INFO)<<"SubGraph size"<<SubGraph.size()<<endl;
    //LOG(INFO)<<"CalPageRank"<<endl;
    CalPageRank_(pr);
    //LOG(INFO)<<"finish"<<endl;
    std::vector<pair<double,int> > TopicPR;
    std::vector<pair<double,string> > RetPR;
    //TopicPR
    TopicPR.resize(limit);
    //TopicPR.resize(limit);
    double lowbound=0.69;
    set<int>::const_iterator sitr = SubGraph.begin();
    for (; sitr != SubGraph.end(); ++sitr)
    {
        int i=(*sitr);

        if(pr.GetContentRelevancy(i)<1)
        {
            pr.SetPr(i,pr.GetPr(i)*30);
        }
        else
        {
            pr.SetPr(i,pr.GetPr(i)*50.0);
        }
        if(nodes_[i]->GetAdvertiRelevancy()>0.1)
        {
            pr.SetPr(i,pr.GetPr(i)*2.0);
        }
       
        if(pr.GetPr(i)>TopicPR[limit-1].first&&pr.GetPr(i)>lowbound)
        {
            //nodes_[i]->PrintNode();
            //pr.GetLinkin(i).PrintNode();
           
            for(unsigned j=0; j<limit; j++)
            {
               
                if((pr.GetPr(i)>TopicPR[j].first))
                {
                    
                    TopicPR.insert(TopicPR.begin()+j ,make_pair(pr.GetPr(i),i) );
                    if(pr.GetPr(i)>0.8)
                    {
                        lowbound=0.7;
                    }
                    break;
                }
            }
        }
    }
    boost::posix_time::ptime time_now = boost::posix_time::microsec_clock::local_time();
    bool dupicate=false;
    for(uint32_t i=0; i<min(TopicPR.size(),limit); i++)
    {
        dupicate=false;
        for(unsigned j=0; j<RetPR.size(); j++)
        {
            if(nodes_[TopicPR[i].second]->GetName()==RetPR[j].second)
            {
                 dupicate=true;
                 break;
            }
        }
        if(dupicate)
        {
                continue;
        }
        RetPR.push_back(make_pair(TopicPR[i].first,nodes_[TopicPR[i].second]->GetName()));

        
    }
    time_now = boost::posix_time::microsec_clock::local_time();
    //LOG(INFO)<<NotInGraph.size()<<endl;
    if(RetPR[0].first<0.7)
    {
        lowbound=0.4;
    }
    for(uint32_t i=0; i<NotInGraph.size(); i++)
    {
        bool dupicate=false;
        for(unsigned j=0; j<min(RetPR.size(),limit); j++)
        {
            if(NotInGraph[i].second==RetPR[j].second)
            {
                //nodes_[i]->PrintNode();
                dupicate=true;
                break;
            }
        }
        if(dupicate)
        {
            continue;
        }
        for(unsigned j=0; j<min(RetPR.size(),limit); j++)
        {
            if(NotInGraph[i].first>RetPR[j].first&&NotInGraph[i].first>lowbound)
            {
                //nodes_[i]->PrintNode();
                RetPR.insert(RetPR.begin()+j ,NotInGraph[i]);
                break;
            }
        }

    }
    time_now = boost::posix_time::microsec_clock::local_time();
    if(RetPR[0].first>=0.6)
    {
        lowbound=0.5;
    }
  
    for(uint32_t i=0; i<min(RetPR.size(),limit); i++)
    {

        if(RetPR[i].first>lowbound)
        {
            if(stopword_.find( RetPR[i].second)==stopword_.end())
            topic_list.push_back(RetPR[i].second);
        }
    }
    time_now = boost::posix_time::microsec_clock::local_time();
    // LOG(INFO)<<"SubGraph size"<<SubGraph.size()<<endl;
    SubGraph.clear();


}

void WikiGraph::InitSubGaph_(const int& index,set<int>& SubGraph,int itertime)
{
    //LOG(INFO)<<nodes_[index]->GetName()<<wa_->Freq(index)<<"  "<<(nodes_[index]->offStop-nodes_[index]->offStart)<<endl;
    if(itertime>2||(itertime!=1&&(wa_->Freq(index)>1000||(nodes_[index]->offStop-nodes_[index]->offStart)>1000))){}
    else if( SubGraph.find(index) == SubGraph.end() )
    {
        SubGraph.insert(index);
        //nodes_[index]->SetPageRank(1.0);
        //vector<int>::const_iterator citr ;//=nodes_[index]->linkin_index_.begin();
        /*
        for (; citr !=nodes_[index]->linkin_index_.end(); ++citr)
        {
             InitSubGaph_(*citr,itertime+1);
        }
        */
        //citr =nodes_[index]->linkout_index_.begin();
        if(itertime==1)
        {
            //LOG(INFO)<<wa_->Freq(index)<<endl;
            int size=wa_->Freq(index);
            if(size==1)
            {
                //if(SubGraph.find(wa_->Select(index,0))!=SubGraph.end())
                    itertime--;
            }
            for (int i=1;i<=size ; i++)
            {
                InitSubGaph_(GetIndexByOffset_(wa_->Select(index,i)-1),SubGraph,itertime+1);
            }
        }

    }
}

void WikiGraph::SetContentBias_(const std::vector<std::pair<std::string,uint32_t> >& relativeWords,PageRank& pr, std::vector<pair<double,string> >& ret)
{
   // LOG(INFO)<<"SetContentBiasBegin"<<endl;
    //vector<pair<string,uint32_t> > RelativeWords;
    //contentBias_.getKeywordFreq(content, RelativeWords);
   // LOG(INFO)<<"RelativeWords:";
   // LOG(INFO)<<relativeWords.size()<<endl;
    for(uint32_t i=0; i<relativeWords.size(); i++)
    {
        int id=GetIdByTitle(relativeWords[i].first);
        // LOG(INFO)<<"id"<<id<<endl;
        if(id>=0)
        {

          //  LOG(INFO)<<"get";
            //LOG(INFO)<<"index"<<getIndex(id)<<endl;
            if(relativeWords[i].second>0.5)
            {
                InitSubGaph_(GetIndex_(id),pr.SubGraph_,1);
            }
            else
            {
                InitSubGaph_(GetIndex_(id),pr.SubGraph_,2);
            }
            if(stopword_.find( relativeWords[i].first)==stopword_.end())
            ret.push_back(make_pair(log(double(advertiseBias_->GetCount(relativeWords[i].first)+1.0))*25*relativeWords[i].second +0.5,nodes_[GetIndex_(id)]->GetName()));
        }
        //LOG(INFO)<<endl;
        //LOG(INFO)<<"id"<<GetIdByTitle(RelativeWords[i].first)<<endl;
        //LOG(INFO)<<"Index"<<getIndex(GetIdByTitle(RelativeWords[i].first))<<endl;
        else
        {
        if(stopword_.find( relativeWords[i].first)==stopword_.end())
            ret.push_back(make_pair(log(double(advertiseBias_->GetCount(relativeWords[i].first)+1.0))*25*relativeWords[i].second +0.5,relativeWords[i].first));
        }
    }
    // LOG(INFO)<<pr.SubGraph_.size()<<endl;
    pr.InitMap();
    // LOG(INFO)<<"init"<<endl;
    for(uint32_t i=0; i<relativeWords.size(); i++)
    {
        int id=GetIdByTitle(relativeWords[i].first);

        if(id>=0)
        {
            if(relativeWords[i].second>0)
            {
                if(advertiseBias_->GetCount(relativeWords[i].first)>0)
                {
                    pr.SetContentRelevancy(GetIndex_(id),relativeWords[i].second+1);
                }
                else
                {
                    pr.SetContentRelevancy(GetIndex_(id),relativeWords[i].second);
                }
            }
            //  if(relativeWords[i].second==0)
            //  pr.SetContentRelevancy(GetIndex_(id),0.1);
        }
    }
    //LOG(INFO)<<"SetContentBiasEnd"<<endl;
    //LOG(INFO)<<endl;
}

Node* WikiGraph::GetNode_(const int&  id,bool& HaveGet)
{
    int  index=GetIndex_(id);
    if(index==(-1))
    {
        HaveGet=false;
        return NULL;
    }
    else
    {
        HaveGet=true;
        return nodes_[index];
    }

}

int WikiGraph::GetIndexByOffset_(const int&  offSet)
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
      // LOG(INFO)<<"mid"<<mid<<endl;
       if(nodes_[mid]->offStart>offSet||nodes_[mid]->offStop<offSet)
       {
         // HaveGet=false;
          LOG(INFO)<<"offset false"<<endl;
          return  -1;
       }
       else
       {
         // HaveGet=true;
          return mid;
       }
}

int WikiGraph::GetIndex_(const int&  id)
{

    int front=0;
    int end=nodes_.size()-1;
    int mid=(front+end)/2;
    while(front<end && nodes_[mid]->GetId()!=id)
    {
        if(nodes_[mid]->GetId()<id)front=mid+1;
        if(nodes_[mid]->GetId()>id)end=mid-1;
        mid=front + (end - front)/2;
    }
    // LOG(INFO)<<"mid"<<mid<<endl;
    if(nodes_[mid]->GetId()!=id)
    {
        // HaveGet=false;
        return  -1;
    }
    else
    {
        // HaveGet=true;
        return mid;
    }
}

Node* WikiGraph::GetNode_(const string&  str,bool& HaveGet)
{
    int id=GetIdByTitle(str);
    // LOG(INFO)<<title2id.size();
    // LOG(INFO)<<str<<"id"<<id<<"   "<<endl;
    if(id==-1)
    {
        return NULL;
    }
    return GetNode_(id,HaveGet);
}

void WikiGraph::SetAdvertiseAll_()
{
    for(uint32_t i=0; i<nodes_.size(); i++)
    {
        // if(i%10000==0){LOG(INFO)<<"Set AdvertiseBias"<<i<<endl;}
        SetAdvertiseBias_(nodes_[i]);
    }
}

void WikiGraph::SetAdvertiseBias_(Node* node)
{
    //node->SetAdvertiRelevancy(advertiseBias_->GetCount()));
    //GetTitleById(node->GetId());
    // node->GetName();
    node->SetAdvertiRelevancy(advertiseBias_->GetCount(node->GetName()));
}

void WikiGraph::CalPageRank_(PageRank& pr)
{
    pr.CalcAll(5);
    //pr_.PrintPageRank(nodes_);
}

void WikiGraph::BuildMap_()
{
/*
    for(unsigned i=0; i<nodes_.size(); i++)
    {

        if(stopword_.find(nodes_[i]->GetName())==stopword_.end())
            title2id.insert(pair<string,int>(nodes_[i]->GetName(),nodes_[i]->GetId()));
        //if(i%10000==0){LOG(INFO)<<"have build map"<<i<<endl;}
        //if(nodes_[i]->GetName()=="奥斯卡"){LOG(INFO)<<i<<"   "<<nodes_[i]->GetId()<<"  "<<Title2Id("奥斯卡")<<endl;}奧斯卡
    }
*/
    ifstream ifs(redirpath_.c_str());
    LoadAll_(ifs);

    // {LOG(INFO)<<"get手机"<<Title2Id("手机")<<endl;}
    // {LOG(INFO)<<"get手機"<<Title2Id("手機")<<endl;}

}

void WikiGraph::InitStopword_()
{
    ifstream in;
    in.open(stopwordpath_.c_str(),ios::in);
    while(!in.eof())
    {
        string word;
        getline(in,word);
        stopword_.insert(word);
    }
}

void WikiGraph::SimplifyTitle_()
{
    for(unsigned i=0; i<nodes_.size(); i++)
    {
        // if(i%10000==0){LOG(INFO)<<"have simplify node"<<i<<endl;}
        simplify(*nodes_[i],opencc_);
    }
}

/*
void WikiGraph::InitOutLink()
{
    for(unsigned i=0; i<nodes_.size(); i++)
    {
        nodes_[i]->linkout_index_.resize(nodes_[i]->outNumber_);
        nodes_[i]->outNumber_=0;

    }
    for(unsigned i=0; i<nodes_.size(); i++)
    {
        for(unsigned j=0; j<nodes_[i]->linkin_index_.size(); j++)
        {
            int k=nodes_[i]->linkin_index_[j];
            nodes_[k]->linkout_index_[nodes_[k]->outNumber_]=i;
            nodes_[k]->outNumber_++;
        }
    }
    for(unsigned i=0; i<nodes_.size(); i++)
    {
        nodes_[i]->outNumber_=nodes_[i]->linkout_index_.size();
    }
}
*/
string WikiGraph::ToSimplified_(const string& name)
{
    std::string lowercase_content = name;
    boost::to_lower(lowercase_content);
    std::string simplified_content;
    long ret = opencc_->convert(lowercase_content, simplified_content);
    if(-1 == ret) simplified_content = lowercase_content;
    return simplified_content;
}

bool WikiGraph::AddTitleIdRelation(const std::string& name, const int& id)
{
    int temp_id;
    std::string temp_name;
    if(stopword_.find(ToSimplified_(name))!=stopword_.end())
      return false;  
    if( !(titleIdDbTable_->get_item(name, temp_id)) ) 
    {
          titleIdDbTable_->add_item(name, id);
    }
    else
    {
          
    }
    if( !(idTitleDbTable_->get_item(id, temp_name)) ) idTitleDbTable_->add_item(id, name);
    return true;
}

std::string WikiGraph::GetTitleById(const int& id)
{
    std::string ret;
    if(idTitleDbTable_->get_item(id, ret))
    return ret;
    else
    return "";
}

int WikiGraph::GetIdByTitle(const std::string& title, const int i)
{
    int id;
    if(stopword_.find(title)!=stopword_.end())
    return -1;
    if(titleIdDbTable_->get_item(title, id))
    {
        map<int,string>::iterator redirit;
        redirit = redirect_.find(id);
        if(redirit !=redirect_.end()&&i<5)
        {
            return  GetIdByTitle(redirit->second,i+1);
        }
        return id;
    }
    return -1;
}

}
