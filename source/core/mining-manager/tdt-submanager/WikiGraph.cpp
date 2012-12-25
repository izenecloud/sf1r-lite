#include "WikiGraph.hpp"
#include <math.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>

namespace sf1r
{

WikiGraph::WikiGraph(const string& cma_path,const string& wiki_path,cma::OpenCC* opencc)
    : contentBias_(cma_path)
    , opencc_(opencc)
{
    boost::filesystem::path wikigraph_path(wiki_path);
    wikigraph_path /= boost::filesystem::path("wikigraph");
    path_ = wikigraph_path.c_str();
    boost::filesystem::path redirect_path(wiki_path);
    redirect_path /= boost::filesystem::path("redirect");
    redirpath_ = redirect_path.c_str();

    //cout<<"wikipediaGraphBuild"<<endl;
    //cout<<"init wiki_path"<<wiki_path<<" "<<path_<<endl;

    init();
    //sort(nodes_.begin(),nodes_end(),NodeCmpOperator);

    cout<<"simplifyTitle...."<<endl;
    simplifyTitle();
    //  flush();
    cout<<"buildMap...."<<endl;
    BuildMap();
    cout<<"SetAdvertise...."<<endl;
    SetAdvertiseAll();
    cout<<"InitOutLink...."<<endl;
    //cout<<"title2id"<<title2id.size()<<endl;
    InitOutLink();
    test();
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

void  WikiGraph::load(std::istream& is)
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

void  WikiGraph::save(std::ostream& os)
{
    unsigned int nodesize=nodes_.size();
    os.write(( char*)&nodesize, sizeof(nodesize));
    for(unsigned int i=0; i<nodes_.size(); i++)
    {
        os<<(*nodes_[i]);
    }

}

void  WikiGraph::init()
{

    std::ifstream ifs(path_.c_str());
    if (ifs) load(ifs);
    ifs.close();
}

void  WikiGraph::flush()
{
    if (path_.empty()) return;
    std::ofstream ofs(path_.c_str());
    if (ofs) save(ofs);
    ofs.close();
}

void  WikiGraph::link2nodes(const int& i,const int& j)
{
    nodes_[i]->InsertLinkInNode(j);
    nodes_[j]->InsertLinkOutNode(i);

}

void  WikiGraph::GetTopics(const std::string& content, std::vector<std::string>& topic_list, size_t limit)
{
    //cout<<"SetContentBias";

    set<int> SubGraph;
    PageRank pr(nodes_,SubGraph);
    std::vector<pair<double,string> > NotInGraph=SetContentBias(content,pr);

    //pr_.PrintPageRank(nodes_);
    cout<<"SubGraph size"<<SubGraph.size()<<endl;
    //cout<<"CalPageRank"<<endl;
    CalPageRank(pr);
    //cout<<"finish"<<endl;
    std::vector<pair<double,string> > TopicPR;

    //TopicPR
    TopicPR.resize(limit);
    double lowbound=0.65;
    set<int>::const_iterator sitr = SubGraph.begin();
    for (; sitr != SubGraph.end(); ++sitr)
    {
        int i=(*sitr);
        if(nodes_[i]->GetAdvertiRelevancy()>0.1)
        {
            pr.setPr(i,pr.getPr(i)+0.5);
        }
        if((pr.getPr(i)>TopicPR[limit-1].first&&nodes_[i]->GetInBoundNum()<10000)||pr.getContentRelevancy(i)*nodes_[i]->GetAdvertiRelevancy()>0.01)
        {
            for(unsigned j=0; j<limit; j++)
            {

                if((pr.getPr(i)>TopicPR[j].first&&pr.getPr(i)>lowbound))
                {
                    //nodes_[i]->PrintNode();
                    TopicPR.insert(TopicPR.begin()+j ,make_pair(pr.getPr(i),nodes_[i]->GetName()) );
                    if(pr.getPr(i)>0.8)
                    {
                        lowbound=0.75;
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
    if(TopicPR[0].first>0.71)
    {
        lowbound=0.7;
    }
    if(TopicPR[0].first>0.8)
    {
        lowbound=0.75;
    }
    // sort(TopicPR.begin(),TopicPR.end());
    //set<int>::const_iterator citr = SubGraph.begin();
    for(uint32_t i=0; i<min(TopicPR.size(),limit); i++)
    {

        if(TopicPR[i].first>lowbound)
        {
            topic_list.push_back(TopicPR[i].second);
            cout<<"result"<<TopicPR[i].second<<"  "<<TopicPR[i].first<<endl;
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

void  WikiGraph::test()
{
    cout<<"test Begin"<<endl;
    vector<string> content;

    content.push_back("手机还是iphone最好,其他手机不行啊");
    content.push_back("韩版秋装新款耸肩镂空短款镂空小立领波浪花边针织开衫外套 ");
    content.push_back("时尚秋冬新款修身韩版秋衣女款长袖打底衫");
    content.push_back("第84屆奥斯卡的那些事兒. - 堆糖专辑");
    content.push_back("【原创手作】树叶杯垫·zakka棉麻。可以订制哦，大家也都可以做起来！！");
    content.push_back("今夏的薄荷绿是主打哦");
    content.push_back("芝心红薯！南方总是吃不到好吃的烤红薯…… 唉 ");
    content.push_back("广东省潮州市的一家电器公司发布了一款限量版iFeng 4S电吹风，这款电吹风上有乔布斯的名字，iFeng 4S与苹果最新的iphone 4S发音相同，公司表示白色是特殊版本，为了向乔布斯献礼，要价99大洋");
    content.push_back("冬季新款韩国原单正品真皮牛皮绑带铆钉女靴欧美复古高跟短靴棕色");
    content.push_back("推荐单鞋圆头平底磨砂面韩版甜美保暖英伦系带毛毛女鞋");
    content.push_back("秋冬新款韩版毛绒带毛毛的皮草包包水貂毛包流苏女包包");
    content.push_back("ATT版nokia lumia 900一览 - 视频 - 优酷视频 - 在线观看-http:");
    content.push_back("韩版秋冬天麻花球状蓓蕾帽 针织帽 毛线帽子 女士 贝雷帽保暖帽潮");
    content.push_back("特价 欧美范羊绒拼色大披肩 保暖流苏围巾 秋冬加厚围脖");
    content.push_back("2012新款女装 欧美秋冬正品 高圆圆 明星同款 气质毛呢大衣 外套");
    content.push_back(" 笑话伤不起：老虎中的怂货。。~（海量精彩GIF图片，请关注@经典GIF图集）");
    content.push_back(" 图 宅男女神刚小希狐媚诱惑-众多知名模特合作推出车模资料库预热大片-新浪汽车论坛");
    content.push_back(" 对于迅鹰你们用什么头盔呢？ - 踏板论坛 - 雅马哈踏板摩托 - 摩托车论坛 - 中国第一摩托车论坛 - 摩旅进行到底!");
    content.push_back("本草拾遗花草茶叶 兰州苦水玫瑰 甘肃特产 紫玫瑰王花茶 100克");
    content.push_back("清仓处理 生态良品zakka 木柄西餐刀 蛋糕刀 牛排刀叉 烘焙餐具 清仓处理 生态良品zakka 木柄西餐刀 蛋糕刀 牛排刀叉 烘焙餐具 ");
    //content.push_back("");

    boost::posix_time::ptime time_now = boost::posix_time::microsec_clock::local_time();
    cout<<"start time:"<<time_now<<endl;


    std::vector<std::string> topic_list;
    for(unsigned i=0; i<content.size(); i++)
    {
        cout<<"content:"<<content[i]<<endl;
        GetTopics(content[i], topic_list, 10);
    }
    cout<<"test End"<<endl;
    time_now = boost::posix_time::microsec_clock::local_time();
    cout<<"end time:"<<time_now<<endl;
}

void  WikiGraph::InitSubGaph(const int& index,set<int>& SubGraph,int itertime)
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
        for (; citr !=nodes_[index]->linkout_index_.end(); ++citr)
        {
            InitSubGaph(*citr,SubGraph,itertime+1);
        }

    }
}

std::vector<pair<double,string> >  WikiGraph::SetContentBias(const string& content,PageRank& pr)
{
    //cout<<"SetContentBiasBegin"<<endl;
    std::vector<pair<double,string> > ret;
    vector<pair<string,uint32_t> > RelativeWords=contentBias_.getKeywordFreq(content);
    // cout<<"RelativeWords:";

    for(uint32_t i=0; i<RelativeWords.size(); i++)
    {
        int id=Title2Id(RelativeWords[i].first);
        // cout<<"id"<<id<<endl;
        if(id>=0)
        {
            //cout<<"index"<<getIndex(id)<<endl;
            InitSubGaph(getIndex(id),pr.SubGraph_,1);
        }
        //cout<<"id"<<Title2Id(RelativeWords[i].first)<<endl;
        //cout<<"Index"<<getIndex(Title2Id(RelativeWords[i].first))<<endl;
        // cout<<RelativeWords[i].first<<" "<<RelativeWords[i].second<<"       ";
        ret.push_back(make_pair(log(double(advertiseBias_.getCount(RelativeWords[i].first)+1.0))*0.25*RelativeWords[i].second +0.5,RelativeWords[i].first));
    }
    pr.InitMap();
    for(uint32_t i=0; i<RelativeWords.size(); i++)
    {
        int id=Title2Id(RelativeWords[i].first);

        if(id>=0)
        {
            pr.setContentRelevancy(getIndex(id),RelativeWords[i].second);
        }
    }
    return ret;
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

Node*  WikiGraph::getNode(const string&  str,bool& HaveGet)
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
void  WikiGraph::SetAdvertiseAll()
{
    for(uint32_t i=0; i<nodes_.size(); i++)
    {
        // if(i%10000==0){cout<<"Set AdvertiseBias"<<i<<endl;}
        SetAdvertiseBias(nodes_[i]);
    }
}

void  WikiGraph::SetAdvertiseBias(Node* node)
{
    node->SetAdvertiRelevancy(advertiseBias_.getCount(node->GetName()) );
};
/* */
int  WikiGraph::Title2Id(const string& title)
{
    map<string, int>::iterator titleit;
    titleit = title2id.find(title);

    if(titleit !=title2id.end())
    {

        return titleit->second;

    }
    else
    {
        return -1;
    }
}

void  WikiGraph::CalPageRank(PageRank& pr)
{
    pr.CalcAll(5);
    //pr_.PrintPageRank(nodes_);
}

void  WikiGraph::BuildMap()
{

    for(unsigned i=0; i<nodes_.size(); i++)
    {

        title2id.insert(pair<string,int>(nodes_[i]->GetName(),nodes_[i]->GetId()));
        //if(i%10000==0){cout<<"have build map"<<i<<endl;}
        //if(nodes_[i]->GetName()=="奥斯卡"){cout<<i<<"   "<<nodes_[i]->GetId()<<"  "<<Title2Id("奥斯卡")<<endl;}奧斯卡
    }
    ifstream ifs(redirpath_.c_str());
    loadAll(ifs);
    // {cout<<"get手机"<<Title2Id("手机")<<endl;}
    // {cout<<"get手機"<<Title2Id("手機")<<endl;}

}

void WikiGraph::simplifyTitle()
{
    for(unsigned i=0; i<nodes_.size(); i++)
    {
        // if(i%10000==0){cout<<"have simplify node"<<i<<endl;}
        simplify(*nodes_[i],opencc_);
    }
}

void  WikiGraph::load(std::istream &f, int& id,string& name )
{
    f.read(( char*)&id, sizeof(id));
    size_t len;
    f.read(( char*)&len, sizeof(len));
    name.resize(len);
    f.read(( char*)&name[0], len);

}

void  WikiGraph::loadAll(std::istream &f )
{
    unsigned size;
    f.read(( char*)&size, sizeof(size));
    for(unsigned i=0; i<size; i++)
    {
        int id;
        string name;
        load(f,id,name);
        // cout<<"name"<<name<<"id"<<id<<endl;
        title2id.insert(pair<string,int>(ToSimplified(name),id));
        // if(i%10000==0){cout<<"have build map"<<nodes_.size()+i<<endl;}
    }
}

void  WikiGraph::InitOutLink()
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
