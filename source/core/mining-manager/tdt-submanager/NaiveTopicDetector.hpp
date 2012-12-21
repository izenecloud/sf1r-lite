#ifndef SF1R_MINING_NAIVE_TOPIC_DETECTOR_HPP_
#define SF1R_MINING_NAIVE_TOPIC_DETECTOR_HPP_

#include <string>
#include <vector>
#include "wikipediaGraph.h"

namespace cma
{
class Analyzer;
class Knowledge;
class OpenCC;
}

namespace sf1r
{
//class wikipediaGraph;
class NaiveTopicDetector
{
public:
    NaiveTopicDetector(const std::string& dict_path,const std::string& cma_path,const std::string& wiki_path);
    ~NaiveTopicDetector();

    bool GetTopics(const std::string& content, std::vector<std::string>& topic_list, size_t limit);
private:
    void InitCMA_();

    std::string tokenize_dicpath_;
    cma::Analyzer* analyzer_;
    cma::Knowledge* knowledge_;
    cma::OpenCC* opencc_;
    wikipediaGraph* wg_;
};
}

#endif

