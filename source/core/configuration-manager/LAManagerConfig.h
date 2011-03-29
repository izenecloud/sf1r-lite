/**
 * @file    LAManagerConfig.h
 * @brief   Defines the LAManagerConfig class
 * @author  MyungHyun (Kent)
 * @date    2008-09-17
 */

#ifndef _LA_MANAGER_CONFIG_H_
#define _LA_MANAGER_CONFIG_H_


#include "TokenizerConfigUnit.h"
#include "LAConfigUnit.h"
#include <la-manager/AnalysisInformation.h>

#include <boost/serialization/array.hpp>
#include <boost/serialization/split_member.hpp>

#include <boost/serialization/map.hpp>

#include <map>
#include <vector>
#include <string>


namespace sf1r
{

/**
 * @brief The class containing the configuration data need for LAManager
 */
class LAManagerConfig
{
public:
    //------------------------  PUBLIC MEMBER FUNCTIONS  ------------------------

    LAManagerConfig()
            : bUseCache_(false)
    {}

    ~LAManagerConfig() {}



    void addTokenizerConfig( const TokenizerConfigUnit & unit )
    {
        tokenizerConfigUnitMap_.insert( std::pair<std::string, TokenizerConfigUnit>(
                                            unit.getId(), unit ));
    }

    void getTokenizerConfigMap( std::map<std::string, TokenizerConfigUnit> & list ) const
    {
        list = tokenizerConfigUnitMap_;
    }

    const std::map<std::string, TokenizerConfigUnit> & getTokenizerConfigMap() const
    {
        return tokenizerConfigUnitMap_;
    }



    void addLAConfig( const LAConfigUnit & unit )
    {
        laConfigUnitMap_.insert( std::pair<std::string, LAConfigUnit>(
                                     unit.getId(), unit ));
    }

    void getLAConfigMap( std::map<std::string, LAConfigUnit> & list ) const
    {
        list = laConfigUnitMap_;
    }

    const std::map<std::string, LAConfigUnit> & getLAConfigMap() const
    {
        return laConfigUnitMap_;
    }


    void setAnalysisPairList( const std::vector<AnalysisInfo> & analysisPairMap )
    {
        analysisPairList_ = analysisPairMap;
    }

    void addAnalysisPair(const AnalysisInfo& pair)
    {
        analysisPairList_.push_back(pair);
    }

    void getAnalysisPairList( std::vector<AnalysisInfo> & analysisPairMap ) const
    {
        analysisPairMap = analysisPairList_;
    }

    const std::vector<AnalysisInfo> & getAnalysisPairList() const
    {
        return analysisPairList_;
    }



    /**
     * @brief Sets whether or not the LAManager will use a cache
     */
    void setUseCache( bool flag )
    {
        bUseCache_ = flag;
    }

    /**
     * @brief Gets whether or not the LAManager will use a cache
     */
    bool getUseCache() const
    {
        return bUseCache_;
    }



private:
    //------------------------  PRIVATE MEMBER FUNCTIONS  ------------------------

    friend class boost::serialization::access;

    template<class Archive>
    void save( Archive & ar, const unsigned int version) const
    {
        /*
        size_t tokenizerSize = tokenizerConfigUnitList_.size();
        size_t laSize = laConfigUnitList_.size();

        ar & tokenizerSize;
        ar & boost::serialization::make_array( &tokenizerConfigUnitList_[0], tokenizerSize );
        ar & laSize;
        ar & boost::serialization::make_array( &laConfigUnitList_[0], laSize );
        */
        ar & tokenizerConfigUnitMap_;
        ar & laConfigUnitMap_;
        ar & analysisPairList_;
        ar & kma_path_;
        ar & bUseCache_;
    }

    template<class Archive>
    void load( Archive & ar, const unsigned int version)
    {
        //size_t tokenizerSize = 0;
        //size_t laSize = 0;

        /*
        ar & tokenizerSize;
        tokenizerConfigUnitList_.resize( tokenizerSize );
        ar & boost::serialization::make_array( &tokenizerConfigUnitList_[0], tokenizerSize );

        ar & laSize;
        laConfigUnitList_.resize( laSize );
        ar & boost::serialization::make_array( &laConfigUnitList_[0], laSize );
        */

        ar & tokenizerConfigUnitMap_;
        ar & laConfigUnitMap_;
        ar & analysisPairList_;
        ar & kma_path_;
        ar & bUseCache_;
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()


public:
    //------------------------  PRIVATE MEMBER VARIABLES  ------------------------


    /// @brief  The list of regulator config units
    std::map<std::string, TokenizerConfigUnit> tokenizerConfigUnitMap_;

    /// @brief  The list of la config units
    std::map<std::string, LAConfigUnit> laConfigUnitMap_;

    /// @brief  A list of all the LAConfig & Regulator pair applied to the properties in config
    std::vector<AnalysisInfo> analysisPairList_;
    /// @brief recode the dictionarypath provided in config file.
    std::string kma_path_;
    /// @brief
    bool bUseCache_;
};




} // namespace

#endif //_LA_MANAGER_CONFIG_H_

// eof
