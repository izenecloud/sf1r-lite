#ifndef SF1R_WIKI_GRAPH_HPP_
#define SF1R_WIKI_GRAPH_HPP_
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include "PageRank.hpp"
#include "AdBias.hpp"
//#include "ConBias.hpp"
//#include "database.h"
#include <algorithm>
#include <icma/icma.h>
#include <icma/openccxx.h>
#include <am/succinct/wat_array/wat_array.hpp>
#include <util/singleton.h>
#include <boost/bimap.hpp>
namespace sf1r
{

class WikiGraph
{
    struct NodeCmp
    {
        bool operator() (Node* node1,Node* node2)
        {
            return node1->GetId() > node2->GetId();
        }

    } NodeCmpOperator;

    std::vector<Node*> nodes_;

    //database db_;
    boost::bimap<std::string, int> title_id_;
    std::map<int,std::string> redirect_;
    // PageRank pr_;
    //set<int> SubGraph_;
    std::string path_;
    std::string redirpath_;
    std::string stopwordpath_;
    //ConBias contentBias_;
    AdBias* advertiseBias_;
    set<string> stopword_;
    wat_array::WatArray wa_;
public:
    static WikiGraph* GetInstance()
    {
        return izenelib::util::Singleton<WikiGraph>::get();
    }

    cma::OpenCC* opencc_;

    WikiGraph();

    ~WikiGraph();

    void Init(const std::string& wiki_path, cma::OpenCC* opencc);

    void GetTopics(const std::vector<std::pair<std::string,uint32_t> >& relativeWords, std::vector<std::string>& topic_list, size_t limit);

    bool AddTitleIdRelation(const std::string& name, const int& id);

    std::string GetTitleById(const int& id);

    int GetIdByTitle(const std::string& title, const int i=0);

protected:
    void SetParam_(const std::string& wiki_path, cma::OpenCC* opencc);

    void Init_();

    void InitStopword_();

    void Flush_();

    void Load_(std::istream& is);

    void Save_(std::ostream& os);

    void LoadAll_(std::istream &f );

    void Load_(std::istream &f, int& id,std::string& name );

    void InitSubGaph_(const int& index,set<int>& SubGraph,int itertime=1);

    void SetContentBias_(const std::vector<std::pair<std::string,uint32_t> >& relativeWords,PageRank& pr,std::vector<pair<double,std::string> >& ret);

    Node* GetNode_(const std::string&  str,bool& HaveGet);

    Node* GetNode_(const int&  id,bool& HaveGet);

    int GetIndex_(const int&  id);

    int GetIndexByOffset_(const int&  offSet);

    void SetAdvertiseAll_();

    void SetAdvertiseBias_(Node* node);

    void CalPageRank_(PageRank& pr);

    void BuildMap_();

    void SimplifyTitle_();

    std::string ToSimplified_(const std::string& name);

};

}

#endif
