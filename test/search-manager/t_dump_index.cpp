#include "t_dump_index.h"

#include <search-manager/ANDDocumentIterator.h>
#include <search-manager/TermDocumentIterator.h>
#include <common/Utilities.h>
#include <boost/filesystem.hpp>
#include <boost/scoped_ptr.hpp>
#include <memory> // auto_ptr
#include <glog/logging.h>
#include <fstream>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


namespace sf1r
{

TestDumpIndex::TestDumpIndex(const std::string& path, int64_t memorySize)
{
    std::string collname = "coll1";
    indexer_.reset(new iii::Indexer);
    iii::IndexManagerConfig indexManagerConfig;
    indexManagerConfig.indexStrategy_.indexLocation_ = path + "/" + collname;
    indexManagerConfig.indexStrategy_.indexMode_ = "default";
    indexManagerConfig.indexStrategy_.memory_ = memorySize;
    indexManagerConfig.indexStrategy_.indexDocLength_ = false;
    indexManagerConfig.indexStrategy_.isIndexBTree_ = false;
    indexManagerConfig.indexStrategy_.skipInterval_ = 8;
    indexManagerConfig.indexStrategy_.maxSkipLevel_ = 3;
    indexManagerConfig.indexStrategy_.indexLevel_ = iii::WORDLEVEL;
    indexManagerConfig.mergeStrategy_.requireIntermediateFileForMerging_ = false;
    indexManagerConfig.mergeStrategy_.param_ = "dbt";
    indexManagerConfig.storeStrategy_.param_ = "file";
    //now collection id always be 1 in IRDatabase.
    uint32_t collectionId = 1;
    std::map<std::string, uint32_t> collectionIdMapping;
    collectionIdMapping.insert(std::pair<std::string, uint32_t>(collname, collectionId));
    iii::IndexerCollectionMeta indexCollectionMeta;
    indexCollectionMeta.setName(collname);

    collmeta_.indexBundleConfig_.reset(new IndexBundleConfiguration(collname));
    collmeta_.productBundleConfig_.reset(new ProductBundleConfiguration(collname));
    collmeta_.miningBundleConfig_.reset(new MiningBundleConfiguration(collname));
    collmeta_.recommendBundleConfig_.reset(new RecommendBundleConfiguration(collname));
    bool ret = CollectionConfig::get()->parseConfigFile(collname, path + "/" + collname + ".xml", collmeta_);
    if (!ret)
    {
        std::cerr << "parse config file failed." << collname;
        exit(-1);
    }
    const IndexBundleSchema& indexSchema = collmeta_.indexBundleConfig_->indexSchema_;
    for (IndexBundleSchema::const_iterator iter = indexSchema.begin(), iterEnd = indexSchema.end();
        iter != iterEnd; ++iter)
    {
        IndexerPropertyConfig indexerPropertyConfig(
            iter->getPropertyId(),
            iter->getName(),
            iter->isIndex(),
            iter->isAnalyzed()
            );
        indexerPropertyConfig.setIsFilter(iter->getIsFilter());
        indexerPropertyConfig.setIsMultiValue(iter->getIsMultiValue());
        indexerPropertyConfig.setIsStoreDocLen(iter->getIsStoreDocLen());
        PropertyDataType sf1r_type = iter->getType();
        izenelib::ir::indexmanager::PropertyType type;
        if(Utilities::convertPropertyDataType(iter->getName(), sf1r_type, type))
        {
            indexerPropertyConfig.setType(type);
        }
        indexCollectionMeta.addPropertyConfig(indexerPropertyConfig);
    }

    indexManagerConfig.addCollectionMeta(indexCollectionMeta);

    indexer_->setIndexManagerConfig(indexManagerConfig, collectionIdMapping);
}

void TestDumpIndex::flush()
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

void TestDumpIndex::prepareDocCommon(uint32_t docId, const std::vector<uint32_t> termid_list,
    iii::IndexerDocument& document)
{
    std::string test_add_prop;
    iii::IndexerPropertyConfig propertyConfig;
    const IndexBundleSchema& indexSchema = collmeta_.indexBundleConfig_->indexSchema_;
    for (IndexBundleSchema::const_iterator iter = indexSchema.begin(), iterEnd = indexSchema.end();
        iter != iterEnd; ++iter)
    {
        if (iter->isIndex() &&
            iter->isAnalyzed() && iter->getName() == "Title")
        {
            propertyConfig.setName(iter->getName());
            propertyConfig.setPropertyId(iter->getPropertyId());
            propertyConfig.setIsIndex(true);
            propertyConfig.setIsAnalyzed(iter->isAnalyzed());
            propertyConfig.setIsFilter(false);
            propertyConfig.setIsMultiValue(iter->getIsMultiValue());
            propertyConfig.setIsStoreDocLen(iter->getIsStoreDocLen());
        }
    }
    if (propertyConfig.getName().empty())
    {
        LOG(INFO) << "no test adding property.";
        return;
    }

    boost::shared_ptr<iii::LAInput> laInput(new iii::LAInput);
    laInput->setDocId(docId);
    laInput->reserve(1024);
    document.insertProperty(propertyConfig, laInput);

    uint32_t pos = 0;
    for (size_t i = 0; i < termid_list.size(); ++i)
    {
        iii::LAInputUnit unit;
        unit.termid_ = termid_list[i];
        unit.docId_ = docId;
        unit.wordOffset_ = pos++;
        document.add_to_property(unit);
    }
}

bool TestDumpIndex::addDoc(uint32_t docId, std::vector<uint32_t>& termid_list)
{
    iii::IndexerDocument document;
    document.setDocId(docId, 1);
    prepareDocCommon(docId, termid_list, document);
    return indexer_->insertDocument(document);;
}

void TestDumpIndex::delDoc(uint32_t docId)
{
    indexer_->removeDocument(1, docId);
}

void TestDumpIndex::updateDoc(uint32_t oldId,
    uint32_t docId, std::vector<uint32_t>& termid_list)
{
    iii::IndexerDocument document;
    document.setDocId(docId, 1);
    document.setOldId(oldId);
    prepareDocCommon(docId, termid_list, document);
    indexer_->updateDocument(document);
}

void TestDumpIndex::optimizeIndex()
{
    indexer_->optimizeIndex();
    indexer_->waitForMergeFinish();
}

void TestDumpIndex::dumpThread(const std::string& basepath, const std::string& property, uint32_t propertyId)
{
    std::cout << "dumping property " << property << " in thread " << boost::this_thread::get_id() << std::endl;

    TermReader *termReader = NULL;
    termReader = indexer_->getIndexReader()->getTermReader(1);
    if (!termReader)
    {
        std::cerr << "get term reader failed.";
        return;
    }
    boost::scoped_ptr<TermIterator> termIterator(
        termReader->termIterator(property.c_str())
        );
    if (!termIterator)
    {
        std::cerr << "get term reader iterator for property failed.";
        return;
    }
    std::ofstream ofs((basepath + property).c_str());
    std::vector<uint32_t> docs;
    //for (uint32_t i = 1000000; i < UINT_MAX; ++i)
    while (termIterator->next())
    {
        const uint32_t i = termIterator->term()->value;
        const TermInfo* terminfo = termIterator->termInfo();
        get(i, property, propertyId, docs);
        if (i % 100000 == 0)
            std::cout << "dumping term : " << i << std::endl;
        if (docs.empty())
            continue;
        ofs << "term id : " << i << ", df :" << terminfo->docFreq_
            << ", lastDocID_:" << terminfo->lastDocID_
            << " posting list: " << std::endl;
        for (size_t j = 0; j < docs.size(); ++j)
        {
            ofs << docs[j] << ",";
        }
        ofs << std::endl;
        std::vector<uint32_t>().swap(docs);
    }
}

void TestDumpIndex::dumpToFile(const std::string& basepath)
{
    std::vector<boost::thread*> running_threads;
    const IndexBundleSchema& indexSchema = collmeta_.indexBundleConfig_->indexSchema_;
    for (IndexBundleSchema::const_iterator iter = indexSchema.begin(), iterEnd = indexSchema.end();
        iter != iterEnd; ++iter)
    {
        if (iter->isIndex() &&
            iter->isAnalyzed())
        {
            boost::thread* t = new boost::thread(boost::bind(&TestDumpIndex::dumpThread, this, basepath, iter->getName(), iter->getPropertyId()));
            running_threads.push_back(t);
        }
        else
        {
            std::cout << "ignore not indexing inverted property." << iter->getName() << std::endl;
        }
    }
    for(size_t i = 0; i < running_threads.size(); ++i)
    {
        if (running_threads[i]->joinable())
            running_threads[i]->join();
        delete running_threads[i];
    }
    running_threads.clear();
    std::cout << "all dump thread finished." << std::endl;
}

bool TestDumpIndex::get(const uint32_t& termid, const std::string& property,
    unsigned int propertyId, std::vector<uint32_t>& docs)
{
    iii::IndexReader* pIndexReader = indexer_->getIndexReader();

    boost::scoped_ptr<ANDDocumentIterator> pAndDocIterator(new ANDDocumentIterator);

    std::auto_ptr<TermDocumentIterator> pTermDocIterator(
        new TermDocumentIterator(
            termid,
            1,
            pIndexReader,
            property,
            propertyId,
            1,
            false)
        );

    if (pTermDocIterator->accept())
    {
        pAndDocIterator->add(pTermDocIterator.release());
    }

    while(pAndDocIterator->next())
    {
        docs.push_back(pAndDocIterator->doc());
    }

    return true;
}

uint32_t TestDumpIndex::get(std::vector<uint32_t>& itemIds, const std::string& property, uint32_t propertyId)
{
    iii::IndexReader* pIndexReader = indexer_->getIndexReader();

    boost::scoped_ptr<ANDDocumentIterator> pAndDocIterator(new ANDDocumentIterator);

    for (std::vector<uint32_t>::iterator p = itemIds.begin(); p != itemIds.end(); p++)
    {
        std::auto_ptr<TermDocumentIterator> pTermDocIterator(
            new TermDocumentIterator(
            (*p),
            1,
            pIndexReader,
            property,
            propertyId,
            1,
            false)
        );
        if (pTermDocIterator->accept())
        {
            pAndDocIterator->add(pTermDocIterator.release());
        }
    }

    if(pAndDocIterator->empty()) return 0;

    uint32_t count = 0;
    while(pAndDocIterator->next())
        ++count;

    return count;
}

size_t TestDumpIndex::getNumItems(const std::string& property)
{
    iii::IndexReader* pIndexReader = indexer_->getIndexReader();
    return pIndexReader->getDistinctNumTerms(1,property);
}

size_t TestDumpIndex::itemFreq(uint32_t itemId, const std::string& property)
{
    iii::IndexReader* pIndexReader = indexer_->getIndexReader();
    iii::Term term(property.c_str(),itemId);
    iii::TermReader* pTermReader = pIndexReader->getTermReader(1);
    if(pTermReader->seek(&term))
        return pTermReader->docFreq(&term);
    else
        return 0;
}

} // namespace sf1r


using namespace sf1r;
BOOST_AUTO_TEST_SUITE( DumpIndex_suite )

BOOST_AUTO_TEST_CASE(test_dump)
{
    if (!boost::filesystem::exists("./sf1config.xml"))
        return;
    SF1Config::get()->setHomeDirectory(".");
    SF1Config::get()->parseConfigFile("./sf1config.xml");

    LOG(INFO) << "testing index and merge";
    boost::filesystem::remove_all("./index_merge_test/coll1");
    //boost::filesystem::create_directories("./index_merge_test");
    {
    TestDumpIndex merge_test("./index_merge_test", 128000000);

    uint32_t test_times = 3;
    uint32_t start_doc = 0;
    uint32_t each_part_num = 100;
    for (size_t cnt = 0; cnt < test_times; ++cnt)
    {
        std::vector<uint32_t> insert_docs_part;
        std::vector<uint32_t> update_docs_part;
        std::vector<uint32_t> del_docs_part;
        for(uint32_t i = start_doc; i < start_doc + each_part_num * 3; ++i)
        {
            if (start_doc == 0)
                continue;
            insert_docs_part.push_back(i);
        }
        // generate the doc id list for update
        for(uint32_t i = start_doc + each_part_num; i < start_doc + each_part_num*2; ++i)
        {
            update_docs_part.push_back(i);
        }
        // generate delete docid list
        for(uint32_t i = start_doc + each_part_num*2; i < start_doc + each_part_num*3; ++i)
        {
            del_docs_part.push_back(i);
        }

        // do index for part1
        for(size_t i = 0; i < insert_docs_part.size(); ++i)
        {
            std::vector<uint32_t> termid_list;
            for(size_t j = 1; j <= insert_docs_part[i]; ++j)
            {
                termid_list.push_back(j);
            }
            merge_test.addDoc(insert_docs_part[i], termid_list);
        }
        for(size_t i = 0; i < update_docs_part.size(); ++i)
        {
            std::vector<uint32_t> termid_list;
            for(size_t j = 1; j <= update_docs_part[i] + each_part_num*2; ++j)
            {
                termid_list.push_back(j);
            }
            merge_test.updateDoc(update_docs_part[i], update_docs_part[i] + each_part_num*2, termid_list);
        }
        for(size_t i = 0; i < del_docs_part.size(); ++i)
        {
            merge_test.delDoc(del_docs_part[i]);
        }

        merge_test.flush();
        start_doc += each_part_num*4;
    }

    LOG(INFO) << "wait 30s to optimizeIndex";
    sleep(30);
    merge_test.optimizeIndex();
    //check merged index.

    merge_test.dumpToFile("./index_merge_test/out-");

    }
    std::cout << "running index test_dump" << std::endl;
    if (!boost::filesystem::exists("./dumpindex_test"))
        return;
    std::cout << "testing dump index: " ;
    TestDumpIndex testdump("./dumpindex_test", 128000000);
    testdump.dumpToFile("./dumpindex_test/out-");
}

BOOST_AUTO_TEST_SUITE_END()
