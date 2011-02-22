/**
 * @file
 * @brief
 * @author
 * @date
 */


#ifndef _RANKING_MANAGER_CONFIG_H_
#define _RANKING_MANAGER_CONFIG_H_

#include <configuration-manager/RankingConfigUnit.h>

#include <boost/serialization/map.hpp>

namespace sf1r
{

    class RankingManagerConfig 
    {
        public:


            /**
             * @brief   Add a collection name and the ranking model associated with it
             */
            bool addRankingConfig( const std::string collName, const RankingConfigUnit & unit )
            {
                return collectionToRankingConfigUnit_.insert( std::pair<std::string, RankingConfigUnit>(collName, unit) ).second;
            }

            /**
             * @brief   Finds the ranking model associated with a collection, given its name
             */
            bool getRankingConfig( const std::string collName, RankingConfigUnit & unit ) const
            { 
                std::map<std::string, RankingConfigUnit>::const_iterator it;

                if( (it = collectionToRankingConfigUnit_.find( collName )) == collectionToRankingConfigUnit_.end() )
                {
                    return false;
                }
                else
                {
                    unit = it->second;
                    return true;
                }
            }



        private:
            //------------------------  PRIVATE MEMBER FUNCTIONS  ------------------------

            friend class boost::serialization::access;

            template<class Archive>
                void serialize( Archive & ar, const unsigned int version) 
                { 
                    ar & collectionToRankingConfigUnit_;
                    ar & propertyWeightMapByProperty_;
                    //ar & collectionMetaNameMap_;
                }


        public:
            /**
             * @brief   Maps a collection name to the ranking model associated with it
             * @details
             * <collection name> to <RankingConfigUnit>
             */
            std::map<std::string, RankingConfigUnit> collectionToRankingConfigUnit_;

            // map<collection name, map<property name, weight of property> >
            std::map<std::string, std::map<std::string, float> >    propertyWeightMapByProperty_;

            /**
             * @details
             * collection name, collection meta
             */
            //std::map<std::string, RankingManagerCollectionMeta> collectionMetaNameMap_;




    };


} // namespace

#endif  //_RANKING_MANAGER_CONFIG_H_

// eof
