#include "NaiveTopicDetector.hpp"
#include <glog/logging.h>
#include <icma/icma.h>
#include <icma/openccxx.h>
#include <la-manager/LAPool.h>
#include <util/ustring/UString.h>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/case_conv.hpp>

namespace sf1r
{
using namespace cma;
using izenelib::util::UString;

NaiveTopicDetector::NaiveTopicDetector(const std::string& dict_path,const std::string& cma_path,const std::string& wiki_path)
    :tokenize_dicpath_(dict_path)
    ,analyzer_(NULL)
    ,knowledge_(NULL)
    ,opencc_(NULL)

{
    InitCMA_();
    wg_=new wikipediaGraph(cma_path,wiki_path,opencc_);
  /*
    for(uint32_t i=0;i<200;i++)
    {
        cout<<"dsadasddas"<<endl;
    }
  */
    //InitCMA_();
}

NaiveTopicDetector::~NaiveTopicDetector()
{
    if (analyzer_) delete analyzer_;
    if (knowledge_) delete knowledge_;
    if (opencc_) delete opencc_;
}

bool NaiveTopicDetector::GetTopics(const std::string& content, std::vector<std::string>& topic_list, size_t limit)
{
   
    std::string lowercase_content = content;
    boost::to_lower(lowercase_content);
    std::string simplified_content;
    long ret = opencc_->convert(lowercase_content, simplified_content);
    wg_->GetTopics(simplified_content, topic_list, limit);
    return true;
    
}

void NaiveTopicDetector::InitCMA_()
{
    std::string cma_path;
    LAPool::getInstance()->get_cma_path(cma_path);
    boost::filesystem::path cma_tdt_dic(cma_path);
    cma_tdt_dic /= boost::filesystem::path(tokenize_dicpath_);
    LOG(INFO) << "topic detector dictionary path : " << cma_tdt_dic.c_str() << endl;
    knowledge_ = CMA_Factory::instance()->createKnowledge();
    knowledge_->loadModel( "utf8", cma_tdt_dic.c_str(), false);
    analyzer_ = CMA_Factory::instance()->createAnalyzer();
    analyzer_->setOption(Analyzer::OPTION_TYPE_POS_TAGGING, 0);
    // using the maxprefix analyzer
    analyzer_->setOption(Analyzer::OPTION_ANALYSIS_TYPE, 100);
    analyzer_->setKnowledge(knowledge_);

    boost::filesystem::path cma_opencc_data_path(cma_path);
    cma_opencc_data_path /= boost::filesystem::path("opencc");
    opencc_ = new OpenCC(cma_opencc_data_path.c_str());
    //wg_.opencc_=opencc_;
    LOG(INFO) << "load dictionary for topic detector knowledge finished." << endl;
}

}
