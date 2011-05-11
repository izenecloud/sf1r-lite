#include "ItemIndex.h"

#include <search-manager/ANDDocumentIterator.h>
#include <search-manager/TermDocumentIterator.h>

#include <boost/filesystem.hpp>
#include <glog/logging.h>

namespace sf1r
{

ItemIndex::ItemIndex(const std::string& path)
        : property_("content")
        , propertyId_(1)
{
    boost::filesystem::create_directories(boost::filesystem::path(path));
    indexer_.reset(new iii::Indexer);
    iii::IndexManagerConfig indexManagerConfig;
    indexManagerConfig.indexStrategy_.indexLocation_ = path;
    indexManagerConfig.indexStrategy_.indexMode_ = "realtime";
    indexManagerConfig.indexStrategy_.memory_ = 10000000;
    indexManagerConfig.indexStrategy_.indexDocLength_ = false;
    indexManagerConfig.indexStrategy_.isIndexBTree_ = false;
    indexManagerConfig.mergeStrategy_.param_ = "default";
    indexManagerConfig.mergeStrategy_.isAsync_ = false;
    indexManagerConfig.storeStrategy_.param_ = "file";
    //now collection id always be 1 in IRDatabase.
    uint32_t collectionId = 1;
    std::map<std::string, uint32_t> collectionIdMapping;
    collectionIdMapping.insert(std::pair<std::string, uint32_t>("coll1", collectionId));
    iii::IndexerCollectionMeta indexCollectionMeta;
    indexCollectionMeta.setName("coll1");
    iii::IndexerPropertyConfig indexerPropertyConfig(kStartFieldId(), property_, true, true);
    indexCollectionMeta.addPropertyConfig(indexerPropertyConfig);
    indexManagerConfig.addCollectionMeta(indexCollectionMeta);

    indexer_->setIndexManagerConfig(indexManagerConfig, collectionIdMapping);
}

void ItemIndex::flush()
{
    try
    {
        indexer_->flush();
    }
    catch (izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::flush(): " << e.what();
    }
}

void ItemIndex::optimize()
{
    indexer_->optimizeIndex();
}

bool ItemIndex::add(ItemIndexDocIDType docId, DataVectorType& doc)
{
    iii::IndexerDocument document;
    document.setDocId(docId,1);
    iii::IndexerPropertyConfig propertyConfig(propertyId_,property_,true,true);

    boost::shared_ptr<iii::LAInput> laInput(new iii::LAInput);
    laInput->setDocId(docId);
    laInput->reserve(1024);
    document.insertProperty(propertyConfig, laInput);

    uint32_t pos = 0;
    for (DataVectorType::iterator it = doc.begin(); it != doc.end(); ++it)
    {
        iii::LAInputUnit unit;
        unit.termid_ = *it;
        unit.docId_ = docId;
        unit.wordOffset_ = pos++;
        document.add_to_property(unit);
    }

    return indexer_->insertDocument(document);;
}

bool ItemIndex::update(ItemIndexDocIDType docId, DataVectorType& doc)
{
    iii::IndexerDocument document;
    document.setDocId(docId,1);
    iii::IndexerPropertyConfig propertyConfig(propertyId_,property_,true,true);

    boost::shared_ptr<iii::LAInput> laInput(new iii::LAInput);
    laInput->setDocId(docId);

    uint32_t pos = 0;
    for (DataVectorType::iterator it = doc.begin(); it != doc.end(); ++it)
    {
        iii::LAInputUnit unit;
        unit.termid_ = *it;
        unit.docId_ = docId;
        unit.wordOffset_ = pos++;
        document.add_to_property(unit);
    }
    document.insertProperty(propertyConfig, laInput);

    return indexer_->updateDocument(document);;
}

bool ItemIndex::del(ItemIndexDocIDType docId)
{
    return indexer_->removeDocument(1, docId);
}

bool ItemIndex::get(std::list<uint32_t>& itemIds, std::list<ItemIndexDocIDType>& docs)
{
    iii::IndexReader* pIndexReader = indexer_->getIndexReader();

    ANDDocumentIterator* pAndDocIterator = new ANDDocumentIterator();

    for (std::list<uint32_t>::iterator p = itemIds.begin(); p != itemIds.end(); p++)
    {
        TermDocumentIterator* pTermDocIterator =
            new TermDocumentIterator(
            (*p),
            1,
            pIndexReader,
            property_,
            propertyId_,
            1,
            false);
        if (pTermDocIterator->accept())
        {
            pAndDocIterator->add(pTermDocIterator);
        }
    }

    if(pAndDocIterator->empty()) return false;

    while(pAndDocIterator->next())
    {
        docs.push_back(pAndDocIterator->doc());
    }

    return true;
}

uint32_t ItemIndex::get(std::vector<uint32_t>& itemIds)
{
    iii::IndexReader* pIndexReader = indexer_->getIndexReader();

    ANDDocumentIterator* pAndDocIterator = new ANDDocumentIterator();

    for (std::vector<uint32_t>::iterator p = itemIds.begin(); p != itemIds.end(); p++)
    {
        TermDocumentIterator* pTermDocIterator =
            new TermDocumentIterator(
            (*p),
            1,
            pIndexReader,
            property_,
            propertyId_,
            1,
            false);
        if (pTermDocIterator->accept())
        {
            pAndDocIterator->add(pTermDocIterator);
        }
    }

    if(pAndDocIterator->empty()) return 0;

    uint32_t count = 0;
    while(pAndDocIterator->next())
        ++count;

    return count;
}

size_t ItemIndex::getNumItems()
{
    iii::IndexReader* pIndexReader = indexer_->getIndexReader();
    return pIndexReader->getDistinctNumTerms(1,property_);
}

} // namespace sf1r

