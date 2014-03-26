#ifndef SF1V5_MINING_SCHEMA_H_
#define SF1V5_MINING_SCHEMA_H_

#include "GroupConfig.h"
#include "AttrConfig.h"
#include "ProductRankingConfig.h"
#include "SuffixMatchConfig.h"
#include "ZambeziConfig.h"
#include "AdIndexConfig.h"

#include <stdint.h>
#include <string>
#include <boost/serialization/access.hpp>

namespace sf1r
{

class MiningSchema
{
public:
    MiningSchema()
        : group_enable(false)
        , attr_enable(false), attr_property()
    {
    }
    ~MiningSchema() {}

private:

    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & group_enable & group_config_map;
        ar & attr_enable & attr_property;
        ar & product_ranking_config;
    }

public:
    bool group_enable;
    GroupConfigMap group_config_map;

    bool attr_enable;
    AttrConfig attr_property;

    ProductRankingConfig product_ranking_config;

    SuffixMatchConfig suffixmatch_schema;

    ZambeziConfig zambezi_config;

    AdIndexConfig ad_index_config;
};

} // namespace

#endif //_MINING_SCHEMA_H_
