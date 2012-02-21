/**
 * @file MultiCollectionItemIdGenerator.h
 * @author Jun Jiang
 * @date 2012-02-17
 */

#ifndef MULTI_COLLECTION_ITEM_ID_GENERATOR_H
#define MULTI_COLLECTION_ITEM_ID_GENERATOR_H

#include "../common/RecTypes.h"

#include <string>
#include <map>
#include <boost/thread/shared_mutex.hpp>

namespace sf1r
{
class ItemIdGenerator;

class MultiCollectionItemIdGenerator
{
public:
    ~MultiCollectionItemIdGenerator();

    /**
     * initialize with a directory path.
     * @param dir the directory path for storage
     * @return true for success, false for failure
     * @post the directory @p dir would be created
     */
    bool init(const std::string& dir);

    bool strIdToItemId(
        const std::string& collection,
        const std::string& strId,
        itemid_t& itemId
    );

    bool itemIdToStrId(
        const std::string& collection,
        itemid_t itemId,
        std::string& strId
    );

    itemid_t maxItemId(const std::string& collection);

private:
    void clear_();

    ItemIdGenerator* getGenerator_(const std::string& collection);
    ItemIdGenerator* findGenerator_(const std::string& collection) const;
    ItemIdGenerator* createGenerator_(const std::string& collection);

private:
    std::string baseDir_;

    /** collection name => ItemIdGenerator instance */
    typedef std::map<std::string, ItemIdGenerator*> CollectionGeneratorMap;
    CollectionGeneratorMap collectionGeneratorMap_;

    mutable boost::shared_mutex collectionMutex_;
};

} // namespace sf1r

#endif // MULTI_COLLECTION_ITEM_ID_GENERATOR_H
