#ifndef SF1R_TDT_CONFIG_H_
#define SF1R_TDT_CONFIG_H_

#include <string>

#include <boost/serialization/access.hpp>

namespace sf1r
{

/**
 * @brief The type of attr property configuration.
 */
class TDTConfig
{
public:
    bool perform_tdt_task;
    std::string tdt_tokenize_dicpath;
    std::string wiki_graph_path;
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize( Archive & ar, const unsigned int version )
    {
        ar & perform_tdt_task;
        ar & tdt_tokenize_dicpath;
      //  ar & wiki_graph_path;
    }
};

} // namespace

#endif //SF1R_TDT_CONFIG_H_
