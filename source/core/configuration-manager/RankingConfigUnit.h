/**
 * @file    RankingConfigUnit
 * @brief
 * @author  MyungHyun Lee (Kent)
 * @date    09-19-2008
 */

#ifndef _RANKING_CONFIG_UNIT_H_
#define _RANKING_CONFIG_UNIT_H_

#include <ranking-manager/RankingEnumerator.h>

#include <boost/serialization/access.hpp>

#include <sstream>
#include <string>

namespace sf1r
{
using namespace sf1r::RankingType;
/**
 * @brief This class describes one ranking setting.
 */
class RankingConfigUnit
{
public:
    RankingConfigUnit()
            : id_()
            , textRankingModel_(NotUseTextRanker)
    {}

    RankingConfigUnit(const std::string& id)
            : id_(id)
            , textRankingModel_(NotUseTextRanker)
    {}

    void setId( const std::string & id )
    {
        id_ = id;
    }
    const std::string& getId() const
    {
        return id_;
    }
    void setTextRankingModel( const TextRankingType & model )
    {
        textRankingModel_ = model;
    }
    TextRankingType getTextRankingModel() const
    {
        return textRankingModel_;
    }

public:

    /**
     * @brief   Puts the configuration into string
     */
    std::string toString()
    {
        std::stringstream ss;
        ss << *this;
        return ss.str();
    }

    friend std::ostream & operator<<(
        std::ostream & out,
        const RankingConfigUnit & unit
    )
    {
        out << "Ranking Configuration: id=" << unit.id_
        << " textmodel=" << unit.textRankingModel_;
        return out;
    }

private:
    friend class boost::serialization::access;

    template <class Archive>
    void serialize( Archive & ar, const unsigned int version )
    {
        ar & id_;
        ar & textRankingModel_;
        // ar & proximityRankingModel_;
        // ar & queryIndependentModel_;
        // ar & boostRankingModel_;
        // ar & qd_ratio;
        // ar & qdqi_ratio_;
        // ar & boostWeight_;
    }

public:

    std::string id_;
    TextRankingType                textRankingModel_;

    // ProximityRankingType           proximityRankingModel_;
    // QueryIndependentRankingType    queryIndependentModel_;
    // BoostRankingType               boostRankingModel_;
    // float qd_ratio;
    // float qdqi_ratio_;
    // float boostWeight_;
};

}
#endif
