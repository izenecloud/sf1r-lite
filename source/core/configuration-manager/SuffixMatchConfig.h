#ifndef SF1V5_MINING_SUFFIXMATCH_CONFIG_H_
#define SF1V5_MINING_SUFFIXMATCH_CONFIG_H_

#include <stdint.h>
#include <string>
#include <boost/serialization/access.hpp>

namespace sf1r
{

struct NumericFilterConfig
{
    NumericFilterConfig()
        : amplifier(1)
        , propType(UNKNOWN_DATA_PROPERTY_TYPE)
    {
    }
    NumericFilterConfig(PropertyDataType type)
        : amplifier(1)
        , propType(type)
    {
    }
    std::string property;
    int32_t amplifier;
    /// property type
    PropertyDataType propType;

    bool isNumericType() const
    {
        return propType == INT32_PROPERTY_TYPE ||
               propType == FLOAT_PROPERTY_TYPE ||
               propType == INT8_PROPERTY_TYPE ||
               propType == INT16_PROPERTY_TYPE ||
               propType == INT64_PROPERTY_TYPE ||
               propType == DOUBLE_PROPERTY_TYPE;
    }

    bool operator<(const NumericFilterConfig& rhs) const
    {
        return property < rhs.property;
    }

    bool operator==(const NumericFilterConfig& rhs) const
    {
        return property == rhs.property;
    }

private:
    friend class boost::serialization::access;
    template <typename Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & property;
        ar & amplifier;
        ar & propType;
    }
};

class SuffixMatchConfig
{
public:
    SuffixMatchConfig()
        : suffix_match_enable(false)
        , suffix_incremental_enable(false)
        , suffix_groupcounter_topk(100000)
    {
    }

    bool suffix_match_enable;
    bool suffix_incremental_enable;
    int32_t suffix_groupcounter_topk;
    std::vector<std::string> suffix_match_properties;
    std::string suffix_match_tokenize_dicpath;
    std::vector<std::string> group_filter_properties;
    std::vector<std::string> attr_filter_properties;
    std::vector<std::string> str_filter_properties;
    std::vector<std::string> date_filter_properties;
    std::vector<NumericFilterConfig> num_filter_properties;
    std::vector<std::string> searchable_properties;

private:
    friend class boost::serialization::access;
    template <typename Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & suffix_match_enable;
        ar & suffix_incremental_enable;
        ar & suffix_groupcounter_topk;
        ar & suffix_match_properties;
        ar & suffix_match_tokenize_dicpath;
        ar & group_filter_properties;
        ar & attr_filter_properties;
        ar & str_filter_properties;
        ar & date_filter_properties;
        ar & num_filter_properties;
        ar & searchable_properties;
    }
};

}

#endif
