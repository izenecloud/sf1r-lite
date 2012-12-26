#include "NaiveTopicDetector.hpp"
#include "WikiGraph.hpp"

#include <glog/logging.h>
#include <icma/icma.h>
#include <icma/openccxx.h>
#include <la-manager/LAPool.h>
#include <util/ustring/UString.h>
#include <util/csv.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <algorithm>

namespace sf1r
{
using namespace cma;
using izenelib::util::UString;

int ReadKeyList(const std::string& fn, vector<std::string>& keyList)
{
    std::ifstream ifs(fn.c_str());
    if (!ifs)
    {
        std::cerr << "cannot open " << fn << std::endl;
        return -1;
      }

    for (std::string key; getline(ifs, key); )
    {
        if (key.size() > 0 && key[key.size()-1] == '\r')
            key = key.substr(0, key.size()-1);
        keyList.push_back(key);
    }
    return 0;
} 

bool SortTopic (const std::pair<std::string, uint32_t>& i,const std::pair<std::string, uint32_t>& j) 
{
    return (i.second > j.second); 
}

bool UniqueTopic (const std::pair<std::string, uint32_t>& i,const std::pair<std::string, uint32_t>& j) 
{
    return (i.first == j.first); 
}

unsigned CommonPrefixLen( std::string a, std::string b )
{
    if( a.size() > b.size() ) std::swap(a,b) ;
    return std::mismatch( a.begin(), a.end(), b.begin() ).first - a.begin();
}

template<typename ForwardIterator, typename BinaryPredicate>
ForwardIterator unique_merge(ForwardIterator first, ForwardIterator last, BinaryPredicate binary_pred)
{
    ForwardIterator result = first;
    while (++first != last)
    {
        if (!(binary_pred(*result, *first)))
            *++result = *first;
        else
            result->second += first->second;
    }
    return ++result;
}

NaiveTopicDetector::NaiveTopicDetector(
        const std::string& sys_resource_path, 
        const std::string& dict_path,
        const std::string& cma_path,
        bool enable_semantic)
    :sys_resource_path_(sys_resource_path)
    ,tokenize_dicpath_(dict_path)
    ,analyzer_(NULL)
    ,knowledge_(NULL)
    ,opencc_(NULL)
    ,wg_(NULL)
    ,kpe_trie_(NULL)
    ,related_map_(NULL)
    ,enable_semantic_(enable_semantic)
{
    InitKnowledge_();
    
}

NaiveTopicDetector::~NaiveTopicDetector()
{
    if (analyzer_) delete analyzer_;
    if (knowledge_) delete knowledge_;
    if (opencc_) delete opencc_;
    if (wg_) delete wg_;
    if (kpe_trie_) delete kpe_trie_;
    if (related_map_) delete related_map_;
}

bool NaiveTopicDetector::GetTopics(const std::string& content, std::vector<std::string>& topic_list, size_t limit)
{
    if(!analyzer_) return false;
    ///convert from traditional chinese to simplified chinese
    std::string lowercase_content = content;
    boost::to_lower(lowercase_content);
    std::string simplified_content;
    long ret = opencc_->convert(lowercase_content, simplified_content);
    if(-1 == ret) simplified_content = lowercase_content;
    std::vector<std::pair<std::string, uint32_t> > topics;
    GetCMAResults_(simplified_content, topics);
    if(wg_)
    {
        if(topics.empty()){return true;}
        if(0 == limit) limit = 5;
        wg_->GetTopics(topics, topic_list, limit);
        return true;
    }
    else
    {
        ///Pure CMA based solution
        std::sort (topics.begin(), topics.end(), SortTopic); 
        size_t size = topics.size();
        if(limit > 0 && limit < topics.size()) size = limit;
        topic_list.reserve(size);
        for(unsigned i = 0; i < size; ++i)
        {
            //LOG(INFO) <<"topic "<<topics[i].first<<" score "<<topics[i].second<<std::endl;    
            topic_list.push_back(topics[i].first);
        }
    }
    return true;
}

void NaiveTopicDetector::GetCMAResults_(const std::string& content, std::vector<std::pair<std::string, uint32_t> >& topics)
{
    UString con_ustr(content, UString::UTF_8);
    if (!con_ustr.includeChineseChar()){return;}
    Sentence pattern_sentence(content.c_str());
    analyzer_->runWithSentence(pattern_sentence);
    //LOG(INFO) << "query tokenize by maxprefix match in dictionary: ";
    for (int i = 0; i < pattern_sentence.getCount(0); i++)
    {
        std::string topic(pattern_sentence.getLexicon(0, i));
        UString topic_ustr(topic, UString::UTF_8);
        //LOG(INFO) <<"topic "<<topic<<std::endl;
        if(topic_ustr.length() > 1)
        {
            izenelib::am::succinct::ux::id_t retID;
            std::vector<std::string> related_keywords;
            retID = related_map_->get(topic.c_str(), topic.size(), related_keywords);
            if(retID == 0)
            {
                unsigned score = 1;//higher weight?
                //LOG(INFO) <<"match related commonlen "<<score<<" related_keywords size "<<related_keywords.size()<<std::endl;
                topics.push_back(std::make_pair(topic,score));
                std::vector<std::string>::iterator rit = related_keywords.begin();
                for(; rit != related_keywords.end(); ++rit)
                {
                    //LOG(INFO) <<"related "<<*rit<<std::endl;
                    topics.push_back(std::make_pair(*rit,0));
                }
            }
            else if(topic_ustr.isAllChineseChar()) /// All Chinese Char is required for a temporary solution.
            {
                size_t retLen;
                retID = kpe_trie_->prefixSearch(topic.c_str(), topic.size(), retLen);
                if(retID != izenelib::am::succinct::ux::NOTFOUND)
                {
                    std::string match = kpe_trie_->decodeKey(retID);
                    //unsigned commonLen = CommonPrefixLen(topic, match);
                    topics.push_back(std::make_pair(topic,1));
                    //LOG(INFO) <<"match "<<match<<" commonlen "<<commonLen<<std::endl;
                }
                else
                    topics.push_back(std::make_pair(topic,1));
            }
        }
    }
    std::sort (topics.begin(), topics.end()); 
    topics.erase( unique_merge( topics.begin(), topics.end(), UniqueTopic), topics.end() );
}

void NaiveTopicDetector::InitKnowledge_()
{
    LOG(INFO) << "topic detector dictionary path : " << tokenize_dicpath_ << endl;
    knowledge_ = CMA_Factory::instance()->createKnowledge();
    knowledge_->loadModel( "utf8", tokenize_dicpath_.c_str(), false);
    analyzer_ = CMA_Factory::instance()->createAnalyzer();
    analyzer_->setOption(Analyzer::OPTION_TYPE_POS_TAGGING, 0);
    // using the maxprefix analyzer
    analyzer_->setOption(Analyzer::OPTION_ANALYSIS_TYPE, 2);
    analyzer_->setKnowledge(knowledge_);

    std::string cma_path;
    LAPool::getInstance()->get_cma_path(cma_path);
    boost::filesystem::path cma_opencc_data_path(cma_path);
    cma_opencc_data_path /= boost::filesystem::path("opencc");
    opencc_ = new OpenCC(cma_opencc_data_path.c_str());

    LOG(INFO) << "load dictionary for topic detector knowledge finished." << endl;

    kpe_trie_ = new izenelib::am::succinct::ux::Trie;
    std::vector<std::string> keyList;
    boost::filesystem::path kpe_dic(tokenize_dicpath_);
    kpe_dic /= boost::filesystem::path("kpe.txt");
    LOG(INFO) << "kpe dictionary path : " << kpe_dic.c_str() << endl;
    ReadKeyList( kpe_dic.c_str(), keyList);
    kpe_trie_->build(keyList);

    related_map_ = new izenelib::am::succinct::ux::Map<std::vector<std::string> >;
    boost::filesystem::path related_map(tokenize_dicpath_);
    related_map /= boost::filesystem::path("relatedkeywords.csv");
    LOG(INFO) << "related keywords dictionary path : " << related_map.c_str() << endl;
    std::ifstream ifs(related_map.c_str());
    if (ifs)
    {
        izenelib::util::csv_parser c(ifs);
        izenelib::util::csv_iterator p(c), q;
        std::map<std::string, std::vector<std::string> > map;
        for (; p!=q; ++p)
        {
            std::vector<std::string> w(p->begin(), p->end());
            unsigned size = w.size();
            if(size > 0)
            {
                std::string k = w[0];
                if(!k.empty())
                {
                    std::vector<std::string> v;
                    for(unsigned i = 1; i < size; ++i)
                    {
                        std::string vv = w[i];
                        if(!vv.empty())
                            v.push_back(vv);
                    }
                    map[k] = v;
                }
            }
        }
        related_map_->build(map);
    }
    else
        LOG(INFO) << "can not open related keywords: " << related_map.c_str() << endl;

    if(enable_semantic_)
    {
        boost::filesystem::path wiki_graph_path(sys_resource_path_);
        wiki_graph_path /= boost::filesystem::path("wikigraph");
        LOG(INFO) << "wiki graph knowledge path : " << wiki_graph_path.c_str() << endl;
        wg_=new WikiGraph(wiki_graph_path.c_str(),opencc_);
    }
}

}
