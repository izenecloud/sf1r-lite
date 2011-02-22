/**
 * @file    DocumentManagerConfig.h
 * @brief   Defines DocumentManagerConfig class
 * @author  MyungHyun Lee (Kent)
 * @date    2008-09-18
 */

#ifndef _DOCUMENT_MANAGER_CONFIG_H_
#define _DOCUMENT_MANAGER_CONFIG_H_

#include "ManagerConfigBase.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>

#include <vector>

namespace sf1r
{

    /**
     * @brief   Container class for the configurations related to document manager
     */
    class DocumentManagerConfig
    {

        public:
            std::string languageIdentifierDbPath_;


        private:
            //------------------------  PRIVATE MEMBER FUNCTIONS  ------------------------

            friend class boost::serialization::access;

            template<class Archive>
                void serialize( Archive & ar, const unsigned int version) 
                { 
                    ManagerConfigBase::serialize( ar, version );
                    ar & languageIdentifierDbPath_;
                    //ar & docSchemaList_;
                }


        private:

            /**
             * @brief   The list of document schema configuraitons defined in the configuration file
             */
            //std::vector<DocumentSchemaConfig> docSchemaList_;

    };

} // namespace
#endif  //_DOCUMENT_MANAGER_CONFIG_H_


// eof
