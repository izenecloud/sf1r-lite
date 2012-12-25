#ifndef SF1R_MINING_NAIVE_TOPIC_DETECTOR_HPP_
#define SF1R_MINING_NAIVE_TOPIC_DETECTOR_HPP_

#include <string>
#include <vector>

#include <am/succinct/ux-trie/uxMap.hpp>

#include "wikipediaGraph.h"

namespace cma
{
class Analyzer;
class Knowledge;
class OpenCC;
}

namespace sf1r
{
class NaiveTopicDetector
{
public:
    NaiveTopicDetector(
        const std::string& sys_resource_path, 
        const std::string& dict_path,
        const std::string& cma_path);

    ~NaiveTopicDetector();

    bool GetTopics(const std::string& content, std::vector<std::string>& topic_list, size_t limit);
private:
    void InitKnowledge_();

    std::string sys_resource_path_;
    std::string tokenize_dicpath_;
    cma::Analyzer* analyzer_;
    cma::Knowledge* knowledge_;
    cma::OpenCC* opencc_;
    wikipediaGraph* wg_;
    izenelib::am::succinct::ux::Trie* kpe_trie_;
    izenelib::am::succinct::ux::Map<std::vector<std::string> >* related_map_;	
};
}

#endif

