#ifndef SF1R_MINING_NAIVE_TOPIC_DETECTOR_HPP_
#define SF1R_MINING_NAIVE_TOPIC_DETECTOR_HPP_

#include <string>
#include <vector>

namespace cma
{
class Analyzer;
class Knowledge;
class OpenCC;
}
namespace izenelib{namespace am{namespace succinct{namespace ux{
class Trie;
}}}}

namespace sf1r
{

class NaiveTopicDetector
{
public:
    NaiveTopicDetector(const std::string& dict_path);
    ~NaiveTopicDetector();

    bool GetTopics(const std::string& content, std::vector<std::string>& topic_list, size_t limit);
private:
    void InitCMA_();

    std::string tokenize_dicpath_;
    cma::Analyzer* analyzer_;
    cma::Knowledge* knowledge_;
    cma::OpenCC* opencc_;

    izenelib::am::succinct::ux::Trie* trie_;
};
}

#endif

