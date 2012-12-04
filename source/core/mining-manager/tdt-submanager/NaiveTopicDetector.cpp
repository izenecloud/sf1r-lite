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
    for (int i = 0; i < pattern_sentence.getCount(0); i++)
    {
        std::string topic(pattern_sentence.getLexicon(0, i));
        UString topic_ustr(topic, UString::UTF_8);
        LOG(INFO) <<"topic "<<topic<<std::endl;
        if(topic_ustr.length() > 1)
            topic_list.push_back(topic);
    }
    if(limit > 0 && limit < topic_list.size())
        topic_list.resize(limit);
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

    LOG(INFO) << "load dictionary for topic detector knowledge finished." << endl;
}

}
