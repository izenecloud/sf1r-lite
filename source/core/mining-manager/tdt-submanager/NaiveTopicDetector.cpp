#include "NaiveTopicDetector.hpp"
#include <glog/logging.h>
#include <icma/icma.h>
#include <icma/openccxx.h>
#include <la-manager/LAPool.h>
#include <util/ustring/UString.h>
#include <am/succinct/ux-trie/uxTrie.hpp>

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

bool SortTopic (const std::pair<std::string, double>& i,const std::pair<std::string, double>& j) 
{
    return (i.second > j.second); 
}

bool UniqueTopic (const std::pair<std::string, double>& i,const std::pair<std::string, double>& j) 
{
    return (i.first == j.first); 
}

unsigned CommonPrefixLen( std::string a, std::string b )
{
    if( a.size() > b.size() ) std::swap(a,b) ;
    return std::mismatch( a.begin(), a.end(), b.begin() ).first - a.begin();
}

NaiveTopicDetector::NaiveTopicDetector(const std::string& dict_path)
    :tokenize_dicpath_(dict_path)
    ,analyzer_(NULL)
    ,knowledge_(NULL)
    ,opencc_(NULL)
{
    InitCMA_();
}

NaiveTopicDetector::~NaiveTopicDetector()
{
    if (analyzer_) delete analyzer_;
    if (knowledge_) delete knowledge_;
    if (opencc_) delete opencc_;
    delete trie_;
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
    Sentence pattern_sentence(simplified_content.c_str());
    analyzer_->runWithSentence(pattern_sentence);
    LOG(INFO) << "query tokenize by maxprefix match in dictionary: ";
    std::vector<std::pair<std::string, double> > topics;
    for (int i = 0; i < pattern_sentence.getCount(0); i++)
    {
        std::string topic(pattern_sentence.getLexicon(0, i));
        UString topic_ustr(topic, UString::UTF_8);
        LOG(INFO) <<"topic "<<topic<<std::endl;
        if(topic_ustr.length() > 1)
        {
            /*
            std::vector<izenelib::am::succinct::ux::id_t> retIDs;  
            // commonPrefixSearch
            trie_->commonPrefixSearch(topic.c_str(), topic.size(), retIDs);
            std::cout << "commonPrefixSearch: " << retIDs.size() << " found." << std::endl;
            for (size_t i = 0; i < retIDs.size(); ++i)
            {
                std::cout << trie_->decodeKey(retIDs[i]) << "\t(id=" << retIDs[i] << ")" << std::endl;
            }
            topics.push_back(std::make_pair(topic,retIDs.size()));*/
            size_t retLen;
            izenelib::am::succinct::ux::id_t retID = trie_->prefixSearch(topic.c_str(), topic.size(), retLen);
            if(retID != izenelib::am::succinct::ux::NOTFOUND)
            {
                std::string match = trie_->decodeKey(retID);
                unsigned commonLen = CommonPrefixLen(topic, match);
                topics.push_back(std::make_pair(topic,commonLen));
                LOG(INFO) <<"match "<<match<<" commonlen "<<commonLen<<std::endl;
            }
            else
                topics.push_back(std::make_pair(topic,0));
        }
    }
    std::sort (topics.begin(), topics.end(), SortTopic); 
    topics.erase( std::unique( topics.begin(), topics.end(), UniqueTopic), topics.end() );

    size_t size = topics.size();
    if(limit > 0 && limit < topics.size()) size = limit;
    topic_list.reserve(size);
    for(unsigned i = 0; i < size; ++i)
        topic_list.push_back(topics[i].first);

    return true;
}

void NaiveTopicDetector::InitCMA_()
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

    trie_ = new izenelib::am::succinct::ux::Trie;
    std::vector<std::string> keyList;
    boost::filesystem::path kpe_dic(tokenize_dicpath_);
    kpe_dic /= boost::filesystem::path("kpe.txt");
    LOG(INFO) << "kpe dictionary path : " << kpe_dic.c_str() << endl;
    ReadKeyList( kpe_dic.c_str(), keyList);
    trie_->build(keyList);
}

}
