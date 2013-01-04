#include "WikiGraph.hpp"
#include <math.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>

namespace sf1r
{

WikiGraph::WikiGraph(const string& wiki_path,cma::OpenCC* opencc)
    : advertiseBias_((boost::filesystem::path(wiki_path)/=boost::filesystem::path("AdvertiseWord.txt")).c_str())
    , opencc_(opencc)
{
    boost::filesystem::path wikigraph_path(wiki_path);
    wikigraph_path /= boost::filesystem::path("wikigraph");
    path_ = wikigraph_path.c_str();
    boost::filesystem::path redirect_path(wiki_path);
    redirect_path /= boost::filesystem::path("redirect");
    redirpath_ = redirect_path.c_str();
    boost::filesystem::path stopword_path(wiki_path);
    stopword_path /= boost::filesystem::path("StopWord.txt");
    stopwordpath_ = stopword_path.c_str();

    //cout<<"wikipediaGraphBuild"<<endl;
    //cout<<"init wiki_path"<<wiki_path<<" "<<path_<<endl;
    initStopword();
    init();
    //sort(nodes_.begin(),nodes_end(),NodeCmpOperator);

    //cout<<"simplifyTitle...."<<endl;
    //cout<<"蘋果 喬布斯"<<endl;
    //simplifyTitle();
    flush();
    cout<<"buildMap...."<<endl;
    BuildMap();
    cout<<"SetAdvertise...."<<endl;
    SetAdvertiseAll();
    cout<<"InitOutLink...."<<endl;
    //cout<<"title2id"<<title2id.size()<<endl;
    InitOutLink();
    //test();

    //pr_.PrintPageRank(nodes_);
    //InitGraph();
    // flush();

    //  init();

    //delete a;

    cout<<"wikipediaGraphDone"<<endl;
}

WikiGraph::~WikiGraph()
{

    for(unsigned i=0; i<nodes_.size(); i++)
    {
        delete nodes_[i];
    }
}

void WikiGraph::load(std::istream& is)
{
    unsigned int nodesize;
    is.read(( char*)&nodesize, sizeof(nodesize));
    nodes_.resize(nodesize);

    for(unsigned int i=0; i<nodesize; i++)
    {
        //if(i%10000==0){cout<<"load "<<i<<"nodes"<<endl;}
        nodes_[i]=new Node("");
        is>>(*nodes_[i]);
    }
}

void WikiGraph::save(std::ostream& os)
{
    unsigned int nodesize=nodes_.size();
    os.write(( char*)&nodesize, sizeof(nodesize));
    for(unsigned int i=0; i<nodes_.size(); i++)
    {
        os<<(*nodes_[i]);
    }

}

void WikiGraph::init()
{

    std::ifstream ifs(path_.c_str());
    if (ifs) load(ifs);
    ifs.close();
}

void WikiGraph::flush()
{
    if (path_.empty()) return;
    std::ofstream ofs("wikigraph");
    if (ofs) save(ofs);
    ofs.close();
}

void WikiGraph::link2nodes(const int& i,const int& j)
{
    nodes_[i]->InsertLinkInNode(j);
    nodes_[j]->InsertLinkOutNode(i);

}

void WikiGraph::GetTopics(const std::vector<std::pair<std::string,uint32_t> >& relativeWords, std::vector<std::string>& topic_list, size_t limit)
{
    //cout<<"SetContentBias";
   // cout<<"word segment end"<<endl;
    set<int> SubGraph;
    PageRank pr(nodes_,SubGraph);
    std::vector<pair<double,string> > NotInGraph;
    SetContentBias(relativeWords,pr,NotInGraph);

    //pr_.PrintPageRank(nodes_);
    //cout<<"SubGraph size"<<SubGraph.size()<<endl;
    //cout<<"CalPageRank"<<endl;
    CalPageRank(pr);
    //cout<<"finish"<<endl;
    std::vector<pair<double,string> > TopicPR;

    //TopicPR
    TopicPR.resize(limit);
    double lowbound=0.69;
    set<int>::const_iterator sitr = SubGraph.begin();
    for (; sitr != SubGraph.end(); ++sitr)
    {
        int i=(*sitr);

        if(pr.getContentRelevancy(i)<1)
        {
            pr.setPr(i,pr.getPr(i)*6);
        }
        else
        {
            pr.setPr(i,pr.getPr(i)*5.0);
        }
        if(nodes_[i]->GetAdvertiRelevancy()>0.1)
        {
            pr.setPr(i,pr.getPr(i)*1.3);
        }
        //nodes_[i]->PrintNode();
        //pr.getLinkin(i).PrintNode();
        if(pr.getPr(i)>TopicPR[limit-1].first)
        {
            bool dupicate=false;
            //cout<<NotInGraph[i].first<<NotInGraph[i].second<<endl;
            if(stopword_.find(nodes_[i]->GetName())!=stopword_.end())
            {
                continue;
            }
            for(unsigned j=0; j<limit; j++)
            {
                if(nodes_[i]->GetName()==TopicPR[j].second)
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
            for(unsigned j=0; j<limit; j++)
            {

                if((pr.getPr(i)>TopicPR[j].first&&pr.getPr(i)>lowbound))
                {

                    TopicPR.insert(TopicPR.begin()+j ,make_pair(pr.getPr(i),nodes_[i]->GetName()) );
                    if(pr.getPr(i)>0.8)
                    {
                        lowbound=0.7;
                    }
                    break;
                }
            }
        }
    }
    //cout<<NotInGraph.size()<<endl;
    if(TopicPR[0].first<0.7)
    {
        lowbound=0.4;
    }
    for(uint32_t i=0; i<NotInGraph.size(); i++)
    {
        bool dupicate=false;
        //cout<<NotInGraph[i].first<<NotInGraph[i].second<<endl;
        for(unsigned j=0; j<limit; j++)
        {
            if(NotInGraph[i].second==TopicPR[j].second)
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
        for(unsigned j=0; j<limit; j++)
        {
            if(NotInGraph[i].first>TopicPR[j].first&&NotInGraph[i].first>lowbound)
            {
                //nodes_[i]->PrintNode();
                TopicPR.insert(TopicPR.begin()+j ,NotInGraph[i]);
                break;
            }
        }

    }
    if(TopicPR[0].first>=0.6)
    {
        lowbound=0.5;
    }
    /*
    if(TopicPR[0].first>0.71)
    {
        lowbound=0.7;
    }
    if(TopicPR[0].first>0.8)
    {
        lowbound=0.75;
    }
    if(TopicPR[0].first>1)
    {
        lowbound=0.8;
    }
    */
    // sort(TopicPR.begin(),TopicPR.end());
    //set<int>::const_iterator citr = SubGraph.begin();
    for(uint32_t i=0; i<min(TopicPR.size(),limit); i++)
    {

        if(TopicPR[i].first>lowbound)
        {
            topic_list.push_back(TopicPR[i].second);
           // cout<<"result"<<TopicPR[i].second<<"  "<<TopicPR[i].first<<endl;
        }
    }
    // SetContentBias(content,pr);
    /*
     for (; citr != SubGraph.end(); ++citr)
     {
        // cout<<"Link in"<<(*citr)<<endl;
         nodes_[(*citr)]->SetPageRank(0.0);
         //node->PrintNode();
        // cout<<"pr"<<pr<<endl;

     }
     */
    SubGraph.clear();
    // cout<<"SubGraph size"<<SubGraph.size()<<endl;

}

void WikiGraph::InitSubGaph(const int& index,set<int>& SubGraph,int itertime)
{
    if(itertime>2||(itertime!=1&&(nodes_[index]->linkin_index_.size()>1000||nodes_[index]->linkout_index_.size()>1000))) {}
    else
    {
        SubGraph.insert(index);
        //nodes_[index]->SetPageRank(1.0);
        vector<int>::const_iterator citr ;//=nodes_[index]->linkin_index_.begin();
        /*
        for (; citr !=nodes_[index]->linkin_index_.end(); ++citr)
        {
             InitSubGaph(*citr,itertime+1);
        }
        */
        citr =nodes_[index]->linkout_index_.begin();
        if(itertime==1)
        {
            if(nodes_[index]->linkout_index_.size()==1)
            {
                if(SubGraph.find(nodes_[index]->linkout_index_[0])!=SubGraph.end())
                    itertime--;
            }
            for (; citr !=nodes_[index]->linkout_index_.end(); ++citr)
            {

                InitSubGaph(*citr,SubGraph,itertime+1);
            }
        }

    }
}

void WikiGraph::SetContentBias(const std::vector<std::pair<std::string,uint32_t> >& relativeWords,PageRank& pr, std::vector<pair<double,string> >& ret)
{
   // cout<<"SetContentBiasBegin"<<endl;
    //vector<pair<string,uint32_t> > RelativeWords;
    //contentBias_.getKeywordFreq(content, RelativeWords);
   // cout<<"RelativeWords:";
   // cout<<relativeWords.size()<<endl;
    for(uint32_t i=0; i<relativeWords.size(); i++)
    {
        int id=Title2Id(relativeWords[i].first);
        // cout<<"id"<<id<<endl;
        //cout<<relativeWords[i].first<<" "<<relativeWords[i].second<<"       ";
        if(id>=0)
        {

          //  cout<<"get";
            //cout<<"index"<<getIndex(id)<<endl;
            if(relativeWords[i].second>0.5)
            {
                InitSubGaph(getIndex(id),pr.SubGraph_,1);
            }
            else
            {
                InitSubGaph(getIndex(id),pr.SubGraph_,2);
            }
        }
        //cout<<endl;
        //cout<<"id"<<Title2Id(RelativeWords[i].first)<<endl;
        //cout<<"Index"<<getIndex(Title2Id(RelativeWords[i].first))<<endl;

        if(stopword_.find( relativeWords[i].first)==stopword_.end())
            ret.push_back(make_pair(log(double(advertiseBias_.GetCount(relativeWords[i].first)+1.0))*0.25*relativeWords[i].second +0.5,relativeWords[i].first));
    }
    pr.InitMap();
    for(uint32_t i=0; i<relativeWords.size(); i++)
    {
        int id=Title2Id(relativeWords[i].first);

        if(id>=0)
        {
            if(relativeWords[i].second>0)
                pr.setContentRelevancy(getIndex(id),relativeWords[i].second);
            //  if(relativeWords[i].second==0)
            //  pr.setContentRelevancy(getIndex(id),0.1);
        }
    }
    //cout<<"SetContentBiasEnd"<<endl;
    //cout<<endl;
}


/*
void  WikiGraph::pairBias(pair<string,uint32_t>& ReLativepair,bool reset)
{
   bool HaveGet;
   cout<<"ReLativepair"<<ReLativepair.first<<" "<<ReLativepair.second;
   Node* node=getNode(ReLativepair.first,HaveGet);
   if(HaveGet&&node)
   {
       cout<<"get";
       if(!reset)
       {
           node->SetContentRelevancy(double(ReLativepair.second));
       }
       else
       {
           node->SetContentRelevancy(0.0);
       }
   }
   cout<<endl;
};
*/
Node*  WikiGraph::getNode(const int&  id,bool& HaveGet)
{
    int  index=getIndex(id);
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

int  WikiGraph::getIndex(const int&  id)
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
    // cout<<"mid"<<mid<<endl;
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

Node* WikiGraph::getNode(const string&  str,bool& HaveGet)
{
    int id=Title2Id(str);
    // cout<<title2id.size();
    // cout<<str<<"id"<<id<<"   "<<endl;
    if(id==-1)
    {
        return NULL;
    }
    return getNode(id,HaveGet);
};
void WikiGraph::SetAdvertiseAll()
{
    for(uint32_t i=0; i<nodes_.size(); i++)
    {
        // if(i%10000==0){cout<<"Set AdvertiseBias"<<i<<endl;}
        SetAdvertiseBias(nodes_[i]);
    }
}

void WikiGraph::SetAdvertiseBias(Node* node)
{
    node->SetAdvertiRelevancy(advertiseBias_.GetCount(node->GetName()) );
};
/* */
int WikiGraph::Title2Id(const string& title,const int i)
{
    map<string, int>::iterator titleit;
    titleit = title2id.find(title);

    if(titleit !=title2id.end())
    {
        map<int,string>::iterator redirit;
        redirit = redirect.find(titleit->second);
        if(redirit !=redirect.end()&&i<5)
        {
           
            return  Title2Id(redirit->second,i+1);
        }
        else
        {
            return titleit->second;
        }
    }
    else
    {
        return -1;
    }
}

void WikiGraph::CalPageRank(PageRank& pr)
{
    pr.CalcAll(5);
    //pr_.PrintPageRank(nodes_);
}

void WikiGraph::BuildMap()
{

    for(unsigned i=0; i<nodes_.size(); i++)
    {

        if(stopword_.find(nodes_[i]->GetName())==stopword_.end())
            title2id.insert(pair<string,int>(nodes_[i]->GetName(),nodes_[i]->GetId()));
        //if(i%10000==0){cout<<"have build map"<<i<<endl;}
        //if(nodes_[i]->GetName()=="奥斯卡"){cout<<i<<"   "<<nodes_[i]->GetId()<<"  "<<Title2Id("奥斯卡")<<endl;}奧斯卡
    }
    ifstream ifs(redirpath_.c_str());
    loadAll(ifs);

    // {cout<<"get手机"<<Title2Id("手机")<<endl;}
    // {cout<<"get手機"<<Title2Id("手機")<<endl;}

}

void WikiGraph::initStopword()
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

void WikiGraph::simplifyTitle()
{
    for(unsigned i=0; i<nodes_.size(); i++)
    {
        // if(i%10000==0){cout<<"have simplify node"<<i<<endl;}
        simplify(*nodes_[i],opencc_);
    }
}

void WikiGraph::load(std::istream &f, int& id,string& name )
{
    f.read(( char*)&id, sizeof(id));
    size_t len;
    f.read(( char*)&len, sizeof(len));
    name.resize(len);
    f.read(( char*)&name[0], len);

}

void WikiGraph::loadAll(std::istream &f )
{
    unsigned size;
    f.read(( char*)&size, sizeof(size));
    for(unsigned i=0; i<size; i++)
    {
        int id;
        string name;
        load(f,id,name);
        // cout<<"name"<<name<<"id"<<id<<endl;
        // if(stopword_.find(ToSimplified(name))==stopword_.end())
        // title2id.insert(pair<string,int>(ToSimplified(name),id));
        redirect.insert(pair<int,string>(id,ToSimplified(name)));
        // if(i%10000==0){cout<<"have build map"<<nodes_.size()+i<<endl;}
    }
}

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

string WikiGraph::ToSimplified(const string& name)
{
    std::string lowercase_content = name;
    boost::to_lower(lowercase_content);
    std::string simplified_content;
    long ret = opencc_->convert(lowercase_content, simplified_content);
    if(-1 == ret) simplified_content = lowercase_content;
    return simplified_content;
}

}
