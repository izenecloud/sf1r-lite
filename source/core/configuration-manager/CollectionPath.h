/** 
 * @file CollectionPath.h
 * @brief Defines and implements CollectionPath class
 * @author MyungHyun (Kent)
 * @date 2009-10-14
 */


#ifndef _SF1V5_COLLECTION_PATH_H_
#define _SF1V5_COLLECTION_PATH_H_

#include <boost/serialization/access.hpp>

#include <string>
#include <stdexcept>


namespace sf1r
{

    /**
     * @brief   This class holds the path information of Collection dependent components
     */
    class CollectionPath
    {
        public:

            /// @brief  Sets the "basepath" of the Collection. This must be set before setting other paths
            /// @param path     The path
            void setBasePath( const std::string & path )
            { 
                basePath_ = path; 
                ensureTrailingSlash(basePath_);
            }

            /// @brief  Gets the basepath
            /// @return The basepath
            std::string getBasePath() const
            {
                return basePath_;
            }


            /// @brief  Sets the "SCD path" of the Collection. The basepath should be set for the default 
            ///         path to be applied
            /// @param path     The path
            void setScdPath( const std::string & path )
            { 
                if( path.empty() )
                    scdPath_ = basePath_ + scdRelativePath_;
                else
                    scdPath_ = path;
                ensureTrailingSlash(scdPath_);
            }

            /// @brief  Gets the scd path
            /// @return The scd path
            std::string getScdPath() const
            {
                return scdPath_;
            }


            /// @brief  Sets the "collection data path" of the Collection. The basepath should be set for the default 
            ///         path to be applied
            /// @param path     The path
            void setCollectionDataPath( const std::string & path )
            { 
                if( path.empty() )
                    collectionDataPath_  = basePath_ + collectionDataRelativePath_;
                else
                    collectionDataPath_ = path;
                ensureTrailingSlash(collectionDataPath_);
            }

            /// @brief  Gets the collection data path
            /// @return The collection data path
            std::string getCollectionDataPath() const
            {
                return collectionDataPath_;
            }

            
            /// @brief  Sets the "query data path" of the Collection. The basepath should be set for the default 
            ///         path to be applied
            /// @param path     The path
            void setQueryDataPath( const std::string & path )
            { 
                if( path.empty() )
                    queryDataPath_   = basePath_ + queryDataRelativePath_;
                else
                    queryDataPath_  = path;
                ensureTrailingSlash(queryDataPath_);
            }

            /// @brief  Gets the query data path
            /// @return The query data path
            std::string getQueryDataPath() const
            {
                return queryDataPath_;
            }


            /// @brief  Sets the "Log path" of the Collection. The basepath should be set for the default 
            ///         path to be applied
            /// @param path     The path
//             void setLogPath( const std::string & path )
//             { 
//                 if( path.empty() )
//                     logPath_ = basePath_ + logRelativePath_;
//                 else
//                     logPath_ = path;
//                 ensureTrailingSlash(logPath_);
//             }

            /// @brief  Gets the log path
            /// @return The log path
//             std::string getLogPath() const
//             {
//                 return logPath_;
//             }


        protected:
            //----------------------------  PROTECTED SERIALIZTAION FUNCTION  ----------------------------  
            friend class boost::serialization::access;

            template <class Archive>
                void serialize( Archive & ar, const unsigned int version )
                {
                    ar & basePath_;
                    ar & scdPath_;
                    ar & collectionDataPath_;
                    ar & queryDataPath_;
                }

        private:

            void ensureTrailingSlash( std::string & str )
            {
#ifdef WIN32
                static const char kSlash = '\\';
#else
                static const char kSlash = '/';
#endif
                if (str.empty())
                {
                    str = ".";
                }

                if (*str.rbegin() != '/' && *str.rbegin() != '\\')
                {
                    str.push_back(kSlash);
                }
            }


        private:
            static const std::string scdRelativePath_;
            static const std::string collectionDataRelativePath_;
            static const std::string queryDataRelativePath_;
            
            /*
            static const char * scdRelativePath_ = "scd";
            static const char * indexRelativePath_ = "index-data";
            static const char * miningRelativePath_ = "mining-data";
            static const char * logRelativePath_ = "log-data";
            */

            std::string basePath_;
            std::string scdPath_;
            std::string collectionDataPath_;
            std::string queryDataPath_;
    };

}

#endif  //_SF1V5_COLLECTION_PATH_H_
