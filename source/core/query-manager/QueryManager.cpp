///
/// @file QueryManager.cpp
/// @brief C++ file of QueryManager class
/// @author Dohyun Yun
/// @date 2008-06-05
/// @details
/// - Log
///     - 2009.06.11 Add getTaxonomyActionItem() by Dohyun Yun
///     - 2009.07.16 Remove considering of the snippet and highlighting option from configuration file.
///     - 2009.08.10 Commented all things in QueryMangaer.cpp and recreate the query manager.
///     - 2009.08.27 Chagne member variable of DocumentClickActionItem class from docIdlist_ to docId_
///     - 2009.09.22 Refactory QueryManager by using extractXXX function.
///     - 2009.10.20 Change the using query correction way.
///

#include "QueryManager.h"
#include <common/SFLogger.h>

namespace sf1r {

std::map<QueryManager::CollPropertyKey_T, sf1r::PropertyDataType> QueryManager::collectionPropertyInfoMap_;
QueryManager::DPM_T QueryManager::displayPropertyMap_;
QueryManager::SPS_T QueryManager::searchPropertySet_;
boost::shared_mutex QueryManager::dpmSM_;

void QueryManager::setCollectionPropertyInfoMap(std::map<QueryManager::CollPropertyKey_T,
        sf1r::PropertyDataType>& collectionPropertyInfoMap )
{
    static boost::mutex localMutex;
    {
        boost::mutex::scoped_lock localLock( localMutex );
        QueryManager::collectionPropertyInfoMap_ = collectionPropertyInfoMap;
    }
}

void QueryManager::addCollectionPropertyInfo(CollPropertyKey_T colPropertyKey,
        sf1r::PropertyDataType  colPropertyData )
{
    static boost::mutex localMutex;
    {
        boost::mutex::scoped_lock localLock( localMutex );
        QueryManager::collectionPropertyInfoMap_[colPropertyKey] = colPropertyData;
    }
}

void QueryManager::addCollectionDisplayProperty(CollPropertyKey_T colPropertyKey,
        DisplayProperty& displayProperty )
{
    boost::upgrade_lock<boost::shared_mutex> lock(dpmSM_);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
    displayPropertyMap_[colPropertyKey] = displayProperty;
}

void QueryManager::addCollectionSearchProperty(CollPropertyKey_T colPropertyKey)
{
    boost::upgrade_lock<boost::shared_mutex> lock(dpmSM_);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
    searchPropertySet_.insert(colPropertyKey);
}

const std::map<QueryManager::CollPropertyKey_T,
      sf1r::PropertyDataType>& QueryManager::getCollectionPropertyInfoMap(void)
{
    return QueryManager::collectionPropertyInfoMap_;
}

void QueryManager::swapPropertyInfo(DPM_T& displayPropertyMap, SPS_T& searchPropertySet)
{
    boost::upgrade_lock<boost::shared_mutex> lock(dpmSM_);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
    displayPropertyMap_.swap( displayPropertyMap );
    searchPropertySet_.swap( searchPropertySet );
} // end - setDisplayPropertyMap()

bool QueryManager::checkDisplayProperty(
        const std::string& collectionName,
        DisplayProperty& displayProperty,
        std::string& errMsg
        ) const
{
    if ( displayProperty.propertyString_ == "DOCID" )
        return true;

    boost::shared_lock<boost::shared_mutex> lock(dpmSM_);
    DPM_T::const_iterator iter =
        displayPropertyMap_.find( make_pair(collectionName, displayProperty.propertyString_) );

    if ( iter == displayPropertyMap_.end() )
    {
        errMsg = "Property(" + displayProperty.propertyString_
            + ") is not found in " + collectionName + " or wrongly set in configuration.";
        return false;
    }

    const DisplayProperty& configDisplayProperty( iter->second );
    if ( configDisplayProperty.isSummaryOn_ == false
            && displayProperty.isSummaryOn_ == true )
    {
        errMsg = "Summary option of Property(" + displayProperty.propertyString_
            + ") is not defined in the configuration.";
        return false;
    }

    if ( configDisplayProperty.isSummaryOn_ == true
            && displayProperty.isSummaryOn_ == true
            && configDisplayProperty.summarySentenceNum_ < displayProperty.summarySentenceNum_ )
    {
        LOG(WARNING) << "Summary Sentence Num of Property "<<displayProperty.propertyString_
            <<" is too big. It will use default num "<< configDisplayProperty.summarySentenceNum_;
        displayProperty.summarySentenceNum_ = configDisplayProperty.summarySentenceNum_;
    }

    if ( configDisplayProperty.isSnippetOn_ == false
            && displayProperty.isSnippetOn_ == true )
    {
        errMsg = "Snippet option of Property(" + displayProperty.propertyString_
            + ") is not defined in the configuration.";
        return false;
    }
    return true;
} // end - checkDisplayProperty()

bool QueryManager::checkDisplayPropertyList(
        const std::string& collectionName,
        std::vector<DisplayProperty>& displayPropertyList,
        std::string& errMsg
        ) const
{
    std::vector<DisplayProperty>::iterator iter = displayPropertyList.begin();
    for(; iter != displayPropertyList.end(); iter++)
    {
        if (!this->checkDisplayProperty( collectionName, *iter, errMsg))
            return false;
    }
    return true;
} // end - checkDisplayPropertyList()

bool QueryManager::checkSearchProperty(
        const std::string& collectionName,
        const std::string& searchProperty,
        std::string& errMsg
        ) const
{
    CollPropertyKey_T key( make_pair(collectionName, searchProperty) );
    if ( searchPropertySet_.find(key) == searchPropertySet_.end() )
    {
        errMsg = "Given Property is not set as an indexed property in the config : " + searchProperty;
        return false;
    }
    return true;
} // end - checkSearchProperty()

bool QueryManager::checkSearchPropertyList(
        const std::string& collectionName,
        const std::vector<std::string>& searchPropertyList,
        std::string& errMsg
        ) const
{
    std::vector<std::string>::const_iterator spIter = searchPropertyList.begin();
    for(; spIter != searchPropertyList.end(); spIter++)
    {
        if ( !checkSearchProperty(collectionName, *spIter, errMsg) )
            return false;
    } // end - for
    return true;
} // end - checkSearchPropertyList()

// ----------------------------------------------------------------------------------

QueryManager::QueryManager()
{
} // end - QueryManager::QueryManager()

QueryManager::~QueryManager()
{
}

} // end - namespace sf1r
