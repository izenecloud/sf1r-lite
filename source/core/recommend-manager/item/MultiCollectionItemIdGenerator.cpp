#include "MultiCollectionItemIdGenerator.h"
#include "SingleCollectionItemIdGenerator.h"
#include "ItemIdGenerator.h"

#include <boost/filesystem.hpp>
#include <glog/logging.h>

namespace bfs = boost::filesystem;

namespace sf1r
{

MultiCollectionItemIdGenerator::~MultiCollectionItemIdGenerator()
{
    clear_();
}

void MultiCollectionItemIdGenerator::clear_()
{
    for (CollectionGeneratorMap::iterator it = collectionGeneratorMap_.begin();
        it != collectionGeneratorMap_.end(); ++it)
    {
        delete it->second;
    }
    collectionGeneratorMap_.clear();
}

bool MultiCollectionItemIdGenerator::init(const std::string& dir)
{
    clear_();

    baseDir_ = dir;
    bfs::create_directories(baseDir_);

    return true;
}

bool MultiCollectionItemIdGenerator::strIdToItemId(
    const std::string& collection,
    const std::string& strId,
    itemid_t& itemId
)
{
    ItemIdGenerator* generator = getGenerator_(collection);
    if (! generator)
        return false;

    return generator->strIdToItemId(strId, itemId);
}

bool MultiCollectionItemIdGenerator::itemIdToStrId(
    const std::string& collection,
    itemid_t itemId,
    std::string& strId
)
{
    ItemIdGenerator* generator = getGenerator_(collection);
    if (! generator)
        return false;

    return generator->itemIdToStrId(itemId, strId);
}

itemid_t MultiCollectionItemIdGenerator::maxItemId(const std::string& collection)
{
    ItemIdGenerator* generator = getGenerator_(collection);
    if (! generator)
        return false;

    return generator->maxItemId();
}

ItemIdGenerator* MultiCollectionItemIdGenerator::getGenerator_(const std::string& collection)
{
    if (collection.empty())
    {
        LOG(ERROR) << "collection name is empty, failed to create ItemIdGenerator";
        return NULL;
    }

    ItemIdGenerator* generator = findGenerator_(collection);
    if (! generator)
    {
        generator = createGenerator_(collection);
    }

    return generator;
}

ItemIdGenerator* MultiCollectionItemIdGenerator::findGenerator_(const std::string& collection) const
{
    boost::shared_lock<boost::shared_mutex> lock(collectionMutex_);

    CollectionGeneratorMap::const_iterator it = collectionGeneratorMap_.find(collection);
    if (it != collectionGeneratorMap_.end())
        return it->second;

    return NULL;
}

ItemIdGenerator* MultiCollectionItemIdGenerator::createGenerator_(const std::string& collection)
{
    boost::unique_lock<boost::shared_mutex> lock(collectionMutex_);

    ItemIdGenerator*& generator = collectionGeneratorMap_[collection];
    if (! generator)
    {
        bfs::path collectionDir = bfs::path(baseDir_) / collection;
        bfs::create_directory(collectionDir);
        generator = new SingleCollectionItemIdGenerator(collectionDir.string());
    }

    return generator;
}

} // namespace sf1r
