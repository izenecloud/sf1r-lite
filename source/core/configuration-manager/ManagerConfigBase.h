#ifndef SF1V5_CONFIGURATION_MANAGER_MANAGER_CONFIG_BASE_H
#define SF1V5_CONFIGURATION_MANAGER_MANAGER_CONFIG_BASE_H
/**
 * @file core/configuration-manager/ManagerConfigBase.h
 * @date Created <2009-10-22 15:07:05>
 * @date Updated <2009-10-22 16:26:36>
 */
#include "CollectionMeta.h"

#include <string>
#include <map>

namespace sf1r {

class ManagerConfigBase
{
    typedef std::map<std::string, CollectionMeta> collection_meta_map;
public:
    typedef collection_meta_map::iterator collection_meta_iterator;
    typedef collection_meta_map::const_iterator collection_meta_const_iterator;

    /**
     * @brief adds one collection meta
     * @param meta collection meta information
     * @return \c true if collection name not exist and \a meta has been added
     *         succesfully.
     */
    bool addCollectionMeta(const CollectionMeta& collectionMeta)
    {
        return collectionMetaNameMap_.insert(
            std::make_pair(collectionMeta.getName(), collectionMeta)
        ).second;
    }

    /**
     * @brief returns a collection meta information with specified
     *        \a collectionName.
     *
     * @param collectionName name of the collection to be searched
     * @param[out] collectionMeta set to the meta information of the collection
     *                            if found.
     * @return \c true if found, \c false otherwise
     */
    bool getCollectionMetaByName(
        const std::string& collectionName,
        CollectionMeta& collectionMeta
    ) const
    {
        std::map<std::string, CollectionMeta>::const_iterator it =
            collectionMetaNameMap_.find(collectionName);

        if(it != collectionMetaNameMap_.end())
        {
            collectionMeta = it->second;
            return true;
        }

        return false;
    }

    collection_meta_iterator collectionMetaBegin()
    {
        return collectionMetaNameMap_.begin();
    }
    collection_meta_iterator collectionMetaEnd()
    {
        return collectionMetaNameMap_.end();
    }
    collection_meta_const_iterator collectionMetaBegin() const
    {
        return collectionMetaNameMap_.begin();
    }
    collection_meta_const_iterator collectionMetaEnd() const
    {
        return collectionMetaNameMap_.end();
    }

    const collection_meta_map& getCollectionMetaNameMap() const
    {
        return collectionMetaNameMap_;
    }

protected:

    //----------------------------  PRIVATE SERIALIZTAION FUNCTION  ----------------------------  
    friend class boost::serialization::access;
    template <typename Archive>
    void serialize( Archive & ar, const unsigned int version )
    {
        ar & collectionMetaNameMap_;
    }

public:
    //----------------------------  PRIVATE MEMBER VARIABLES  ----------------------------  

    /// @brief  Maps Collection name to Collection meta information
    std::map<std::string, CollectionMeta> collectionMetaNameMap_;
};

template<typename InputIterator>
bool addCollectionMeta(InputIterator first, InputIterator last,
                       ManagerConfigBase& result)
{
    bool ret = true;
    for (InputIterator it = first; it != last; ++it)
    {
        ret &= result.addCollectionMeta(*it);
    }

    return ret;
}

} // namespace sf1r

#endif // SF1V5_CONFIGURATION_MANAGER_MANAGER_CONFIG_BASE_H
