#ifndef SF1R_MINING_NAIVE_TOPIC_DETECTOR_HPP_
#define SF1R_MINING_NAIVE_TOPIC_DETECTOR_HPP_

#include <string>
#include <vector>

#include <am/succinct/ux-trie/uxMap.hpp>

namespace cma
{
class Analyzer;
class Knowledge;
class OpenCC;
}

namespace sf1r
{
class WikiGraph;
class NaiveTopicDetector
{
public:
    NaiveTopicDetector(
        const std::string& sys_resource_path, 
        const std::string& dict_path,
        bool enable_semantic = false);

    ~NaiveTopicDetector();

    bool GetTopics(const std::string& content, std::vector<std::string>& topic_list, size_t limit);
private:
    void GetCMAResults_(const std::string& content, std::vector<std::pair<std::string, uint32_t> >& topics);

    void InitKnowledge_();

    std::string sys_resource_path_;
    std::string tokenize_dicpath_;
    cma::Analyzer* analyzer_;
    cma::Knowledge* knowledge_;
    cma::OpenCC* opencc_;
    WikiGraph* wg_;
    izenelib::am::succinct::ux::Trie* kpe_trie_;
    izenelib::am::succinct::ux::Map<std::vector<std::string> >* related_map_;	
    bool enable_semantic_;
};
}

#endif

