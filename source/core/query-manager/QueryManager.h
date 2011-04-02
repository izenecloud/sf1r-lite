///
/// @file QueryManager.h
/// @brief header file of QueryManager
/// @author Dohyun Yun
/// @date 2008-06-05
/// @details
/// - Log
///     - 2009.06.11 Add getTaxonomyActionItem() by Dohyun Yun
///     - 2009.08.10 Comment All things and rebuild the query-manager
///     - 2009.08.27 Added Query Correction SubManager.
///     - 2009.08.31 Added getSimilarClickActionItem.
///

#if !defined(_QUERY_MANAGER_)
#define _QUERY_MANAGER_


#include "ActionItem.h"

#include <util/ticpp/tinyxml.h>

#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/thread.hpp>

#include <vector>

namespace sf1r {

class QueryManager
{
    public: // typedef

        typedef pair<std::string , std::string> CollPropertyKey_T; // Collection Property Name Key Type
        typedef boost::unordered_map<CollPropertyKey_T,DisplayProperty> DPM_T; // Display Property Map Type
        typedef boost::unordered_set<CollPropertyKey_T> SPS_T; // Search Property Set Type

    public: // public ()

        QueryManager();

        ~QueryManager();

    public: // public static ()

        static void setCollectionPropertyInfoMap(std::map<CollPropertyKey_T, 
                sf1r::PropertyDataType>& collectionPropertyInfoMap );

        static const std::map<CollPropertyKey_T, 
                     sf1r::PropertyDataType>& getCollectionPropertyInfoMap(void);

        static void swapPropertyInfo(DPM_T& displayPropertyMap, SPS_T& searchPropertySet);

//         static bool getRefinedQuery(const RequesterEnvironment& env, std::string& refinedQuery);

	static void addCollectionPropertyInfo(CollPropertyKey_T colPropertyKey, 
			sf1r::PropertyDataType colPropertyData );
	
	static void addCollectionDisplayProperty(CollPropertyKey_T colPropertyKey, 
			DisplayProperty& displayProperty );
	
	static void addCollectionSearchProperty(CollPropertyKey_T colPropertyKey);

    private: // private ()

        bool checkDisplayProperty(
                const std::string& collectionName, 
                DisplayProperty& displayProperty,
                std::string& errMsg
                ) const;

        bool checkDisplayPropertyList(
                const std::string& collectionName,
                std::vector<DisplayProperty>& displayPropertyList,
                std::string& errMsg
                ) const;

        bool checkSearchProperty(
                const std::string& collectionName,
                const std::string& searchProperty,
                std::string& errMsg
                ) const;

        bool checkSearchPropertyList(
                const std::string& collectionName, 
                const std::vector<std::string>& searchPropertyList,
                std::string& errMsg
                ) const;

    public: // public static variables

        static const std::string seperatorString;

    private: // private static variables

        

        static std::map<CollPropertyKey_T, sf1r::PropertyDataType> collectionPropertyInfoMap_;

        static DPM_T displayPropertyMap_;
        static boost::shared_mutex dpmSM_; // display property map shared mutex

        static SPS_T searchPropertySet_;

}; // end - class QueryManager


} // end - namespace sf1r

#endif // _QUERY_MANAGER_
