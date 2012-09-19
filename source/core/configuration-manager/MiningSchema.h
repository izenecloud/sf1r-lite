#ifndef SF1V5_MINING_SCHEMA_H_
#define SF1V5_MINING_SCHEMA_H_

#include "GroupConfig.h"
#include "AttrConfig.h"
#include "ProductRankingConfig.h"
#include "SummarizeConfig.h"

#include <stdint.h>
#include <string>
#include <boost/serialization/access.hpp>

namespace sf1r
{

class MiningSchema
{
public:
    MiningSchema()
        : tg_enable(false), tg_kpe_only(false)
        , dupd_enable(false), dupd_fp_only(false)
        , sim_enable(false)
        , dc_enable(false)
        , faceted_enable(false)
        , group_enable(false)
        , attr_enable(false), attr_property()
        , ise_enable(false)
        , recommend_tg(false), recommend_querylog(true)
        , summarization_enable(false), summarization_schema()
        , suffix_match_enable(false)
    {
    }
    ~MiningSchema() {}

private:

    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & tg_enable & tg_kpe_only & tg_properties;
        ar & dupd_enable & dupd_fp_only & dupd_properties;
        ar & sim_enable & sim_properties;
        ar & dc_enable & dc_properties & faceted_enable & faceted_properties;
        ar & group_enable & group_config_map;
        ar & attr_enable & attr_property;
        ar & product_ranking_config;
        ar & tdt_enable;
        ar & ise_enable & ise_property;
        ar & recommend_tg & recommend_querylog & recommend_properties;
        ar & summarization_enable & summarization_schema;
        ar & suffix_match_enable & suffix_match_property;
    }

public:
    bool tg_enable;
    bool tg_kpe_only;
    std::vector<std::string> tg_properties;

    bool dupd_enable;
    bool dupd_fp_only;
    std::vector<std::string> dupd_properties;

    bool sim_enable;
    std::vector<std::string> sim_properties;

    bool dc_enable;
    std::vector<std::string> dc_properties;

    bool faceted_enable;
    std::vector<std::string> faceted_properties;

    bool group_enable;
    GroupConfigMap group_config_map;

    bool attr_enable;
    AttrConfig attr_property;

    ProductRankingConfig product_ranking_config;

    bool tdt_enable;

    bool ise_enable;
    std::string ise_property;

    bool recommend_tg;
    bool recommend_querylog;
    std::vector<std::string> recommend_properties;

    bool summarization_enable;
    SummarizeConfig summarization_schema;

    bool suffix_match_enable;
    std::string suffix_match_property;
};

} // namespace

#endif //_MINING_SCHEMA_H_
