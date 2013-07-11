#include <document-manager/text-summarization-submanager/TextSummarizationSubManager.h>
#include <document-manager/DocumentManager.h>
#include <document-manager/Document.h>
#include <la-manager/LAManager.h>
#include <aggregator-manager/IndexWorker.h>
#include <index/IndexBundleConfiguration.h>
#include <fstream>
#include <algorithm>
#include <util/ustring/UString.h>
#include <ir/id_manager/IDManager.h>
#include <boost/filesystem.hpp>

#include <util/ustring/UString.h>

#include <stdlib.h>

#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/ref.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/random.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>
#include <common/Utilities.h>
#include <iostream>
#include <fstream>
#include <map>


using namespace std;
using namespace boost;

using namespace sf1r;

namespace bfs = boost::filesystem;

izenelib::util::UString::EncodingType encoding = izenelib::util::UString::UTF_8;

string alpha="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
boost::mt19937 generator(static_cast<unsigned int>(std::time(0)));
boost::uniform_int<> uni_int_dist( 0, alpha.length() );
boost::variate_generator< boost::mt19937&, boost::uniform_int<> > r( generator, uni_int_dist );

void clearFiles()
{
    bfs::path dmPath(bfs::path(".") /"document");
    boost::filesystem::remove_all(dmPath);
    bfs::path idPath(bfs::path(".") /"id");
    boost::filesystem::remove_all(idPath);
    bfs::path indexPath(bfs::path(".") /"index");
    boost::filesystem::remove_all(indexPath);
    bfs::path dm1Path(bfs::path(".") /"document1");
    boost::filesystem::remove_all(dm1Path);
}

void makeSchema(IndexBundleSchema& indexSchema)
{
    PropertyConfig pconfig1;
    pconfig1.setName("DOCID");
    PropertyConfig pconfig2;
    pconfig2.setName("DATE");
    PropertyConfig pconfig3;
    pconfig3.setName("Title");
    pconfig3.setIsIndex(true);
    pconfig3.setType(sf1r::STRING_PROPERTY_TYPE);
    AnalysisInfo analysisInfo;
    analysisInfo.analyzerId_ = "la_sia";
    pconfig3.setAnalysisInfo(analysisInfo);
    PropertyConfig pconfig4;
    pconfig4.setName("Content");
    PropertyConfig pconfig5;
    pconfig5.setName("ImgURL");

    indexSchema.insert(pconfig1);
    indexSchema.insert(pconfig2);
    indexSchema.insert(pconfig3);
    indexSchema.insert(pconfig4);
    indexSchema.insert(pconfig5);
}


boost::shared_ptr<DocumentManager>
createDocumentManager()
{
    bfs::path dmPath(bfs::path(".") /"document/");
    bfs::create_directories(dmPath);
    IndexBundleSchema indexSchema;
    makeSchema(indexSchema);
    boost::shared_ptr<DocumentManager> ret(
        new DocumentManager(
            dmPath.string(),
            indexSchema,
            encoding,
            2000
        )
    );
    return ret;
}

boost::shared_ptr<DocumentManager>
createDocumentManager1()
{
    bfs::path dmPath(bfs::path(".") /"document1/");
    bfs::create_directories(dmPath);
    IndexBundleSchema indexSchema;
    makeSchema(indexSchema);
    boost::shared_ptr<DocumentManager> ret(
        new DocumentManager(
            dmPath.string(),
            indexSchema,
            encoding,
            2000
        )
    );
    return ret;
}

void buildProperty(Document::doc_prop_value_strtype& string, int iLength)
{
    for( int i = 0 ; i < iLength ; ++i )
        string += alpha[r()];//random
}

void buildDOCID(izenelib::util::UString& string, int iLength)
{
    //
}

void prepareDocument(unsigned int docId, Document& document)
{
    document.setId(docId);
    Document::doc_prop_value_strtype property;
    buildProperty(property, 5);
    document.property("DOCID") = property;
    property.clear();
    buildProperty(property, 10);
    document.property("DATE") = property;
    property.clear();
    buildProperty(property, 100);
    document.property("Title") = property;
    property.clear();
    buildProperty(property, 2000);
    document.property("Content") = property;
    property.clear();
    buildProperty(property, 50);
    document.property("ImgURL") = property;
}

void prepareDefinedDocument(unsigned int docId, Document& document, const string& value )
{
    document.setId(docId);
    Document::doc_prop_value_strtype property;
    buildProperty(property, 5);
    document.property("DOCID") = property;
    property.clear();
    buildProperty(property, 10);
    document.property("DATE") = property;
    //property.clear();
    //buildProperty(property, 100);
    document.property("Title") = str_to_propstr(value);
    //property.clear();
    //buildProperty(property, 2000);
    document.property("Content") = str_to_propstr(value + value);
    property.clear();
    buildProperty(property, 50);
    document.property("ImgURL") = property;
}

BOOST_AUTO_TEST_SUITE(DocumentManager_test)

BOOST_AUTO_TEST_CASE(insert)
{
    clearFiles();

    boost::shared_ptr<DocumentManager> documentManager = createDocumentManager();

    for(unsigned int i = 1; i <= 1000; ++i)
    {
        Document document;
        prepareDocument(i, document);
        BOOST_CHECK_EQUAL(documentManager->insertDocument(document),true);
    }
}

BOOST_AUTO_TEST_CASE(update)
{
    clearFiles();
    boost::shared_ptr<DocumentManager> documentManager = createDocumentManager();
    for(unsigned int i = 1; i <= 1000; ++i)
    {
        Document document;
        prepareDocument(i, document);
        BOOST_CHECK_EQUAL(documentManager->insertDocument(document),true);
    }

    documentManager.reset();//where??
    documentManager = createDocumentManager();
    for(unsigned int i = 1; i <= 1000; ++i)
    {
        Document document;
        prepareDocument(i, document);
        BOOST_CHECK_EQUAL(documentManager->updateDocument(document),true);
    }

}

BOOST_AUTO_TEST_CASE(remove)
{
    clearFiles();

    boost::shared_ptr<DocumentManager> documentManager = createDocumentManager();
    for(unsigned int i = 1; i <= 10000; ++i)
    {
        Document document;
        prepareDocument(i, document);
        BOOST_CHECK_EQUAL(documentManager->insertDocument(document),true);
    }

    for(unsigned int i = 1; i <= 1000; ++i)
    {
        BOOST_CHECK_EQUAL(documentManager->removeDocument(i),true);
    }

    for(unsigned int i = 3001; i <= 4000; ++i)
    {
        BOOST_CHECK_EQUAL(documentManager->removeDocument(i),true);
    }
    
    for(unsigned int i = 10001; i <= 20000; ++i)
    {
        Document document;
        prepareDocument(i, document);
        BOOST_CHECK_EQUAL(documentManager->insertDocument(document),true);
    }
    
    for(unsigned int i = 1; i <= 1000; ++i)
    {
	 Document document;
         BOOST_CHECK_EQUAL(documentManager->getDocument(i, document),false);
         BOOST_CHECK_EQUAL(documentManager->isDeleted(i), true);
    }

    for(unsigned int i = 3001; i <= 4000; ++i)
    {
	 Document document;
         BOOST_CHECK_EQUAL(documentManager->getDocument(i, document),false);
         BOOST_CHECK_EQUAL(documentManager->isDeleted(i), true);
    }
    
    for(unsigned int i = 4001; i <= 20000; ++i)
    {
	 Document document;
         BOOST_CHECK_EQUAL(documentManager->getDocument(i, document),true);
         BOOST_CHECK_EQUAL(documentManager->isDeleted(i), false);
    }

    uint32_t minDocId = 1;
    uint32_t maxDocId = documentManager->getMaxDocId();
    BOOST_CHECK_EQUAL(maxDocId, 20000);

    //check pretent rebuild an DocumentManager...
    boost::shared_ptr<DocumentManager> documentManager1 = createDocumentManager1();

    uint32_t i = 0;
    uint32_t newid = 0;
    for(i = minDocId; i <= maxDocId; i++)
    {
        if(documentManager->isDeleted(i))
            continue;
        newid++;
        Document document;
        documentManager->getDocument(i, document);
        prepareDocument(newid, document);
        documentManager1->insertDocument(document);
    }
    documentManager1->flush();
    for(i = 1; i <= 18000; i++)
    {
        Document document;
        BOOST_CHECK_EQUAL(documentManager1->getDocument(i, document), true);
    }
 
    for(i = 19901; i <= 20000; i++)
    {
        Document document;
        BOOST_CHECK_EQUAL(documentManager1->getDocument(i, document), false);
    }
}

BOOST_AUTO_TEST_CASE(Rebuild)
{
    /*clearFiles();

    boost::shared_ptr<DocumentManager> documentManager = createDocumentManager();
    for(unsigned int i = 1; i <= 10000; ++i)
    {
        Document document;
        prepareDocument(i, document);
        BOOST_CHECK_EQUAL(documentManager->insertDocument(document),true);
    }
    //in indexWorker. the Random IO will be changed into Sequential IO, UPDATE_BUFFER_CAPACITY for flush documents is 8192
    for(unsigned int i = 1; i <= 1808; i++)
    {
        Document document;
        BOOST_CHECK_EQUAL(documentManager->removeDocument(i),true);
        BOOST_CHECK_EQUAL(documentManager->getDocument(i, document),false);
    }

    std::string dir = "./id/";
    boost::shared_ptr<DocumentManager> documentManager1 = createDocumentManager1();

    boost::filesystem::create_directories(dir);
    boost::shared_ptr<IDManager> idManager(new IDManager(dir));
    dir = "./index/";
    boost::filesystem::create_directories(dir);
    boost::shared_ptr<IndexManager> indexManager(new IndexManager());
    boost::shared_ptr<LAManager> laManager(new LAManager());
    std::vector<AnalysisInfo> analysisInfoList;
    LAManagerConfig config;
    initConfig( analysisInfoList, config );

    LAPool::getInstance()->initLangAnalyzer();
    LAPool::getInstance()->init(config);
    std::string collectionname = "test";
    IndexBundleSchema indexSchema;
    if (indexManager)
    {
        izenelib::ir::indexmanager::IndexManagerConfig config;
        config.indexStrategy_.indexLocation_ = dir;
        IndexerCollectionMeta indexCollectionMeta;
        indexCollectionMeta.setName(collectionname);
        makeSchema(indexSchema);
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
            if(sf1r::Utilities::convertPropertyDataType(iter->getName(), sf1r_type, type))
            {
                indexerPropertyConfig.setType(type);
            }
            indexCollectionMeta.addPropertyConfig(indexerPropertyConfig);
        }
        config.addCollectionMeta(indexCollectionMeta);
        std::map<std::string, unsigned int> collectionIdMapping;
        collectionIdMapping[collectionname] = 1;

        indexManager->setIndexManagerConfig(config, collectionIdMapping);
    }
    else
        return;

    IndexBundleConfiguration* indexBundleConfig = new IndexBundleConfiguration(collectionname);
    indexBundleConfig->indexSchema_ = indexSchema;
    DirectoryRotator directoryRotator;    
    boost::shared_ptr<IndexWorker> indexWorker(new IndexWorker(indexBundleConfig, directoryRotator, indexManager));
    indexWorker->setManager_test(idManager, documentManager1, indexManager, laManager);
    //for reindex in indexWoker, we need extra resource and index datas, and LA resource.
    //indexWorker->reindex(documentManager);

    //check idManager_
    uint32_t maxDocId = idManager->getMaxDocId();
    //BOOST_CHECK_EQUAL(maxDocId, 8192);
    maxDocId = documentManager1->getMaxDocId();
    //BOOST_CHECK_EQUAL(maxDocId, 8192);
    
    for(unsigned int i = 1; i <= 8; ++i)
    {
        Document document;
       // BOOST_CHECK_EQUAL(documentManager1->getDocument(i, document),true);
    }
     for(unsigned int i = 8193; i <= 10000; ++i)
    {
        Document document;
        //BOOST_CHECK_EQUAL(documentManager1->getDocument(i, document),false);
    }*/
}

BOOST_AUTO_TEST_CASE(get)
{
    clearFiles();
    boost::shared_ptr<DocumentManager> documentManager = createDocumentManager();
    for(unsigned int i = 1; i <= 1000; ++i)
    {
        Document document;
        prepareDocument(i, document);
        BOOST_CHECK_EQUAL(documentManager->insertDocument(document),true);
    }

    for(unsigned int i = 1; i <= 1000; ++i)
    {
        Document document;
        BOOST_CHECK_EQUAL(documentManager->getDocument(i, document),true);
    }

}

BOOST_AUTO_TEST_CASE(summary)
{
    clearFiles();
    boost::shared_ptr<DocumentManager> documentManager = createDocumentManager();
    std::vector<sf1r::docid_t> docIdList;
    for(unsigned int i = 1; i <= 10; ++i)
    {
        Document document;
        prepareDefinedDocument(i, document, "a apple and a ball");
        BOOST_CHECK_EQUAL(documentManager->insertDocument(document),true);
        docIdList.push_back( i );
    }

    std::vector<izenelib::util::UString> queryTermString;
    queryTermString.push_back( izenelib::util::UString( "a" , izenelib::util::UString::UTF_8) );

    std::vector<izenelib::util::UString> outSnippetList;
    std::vector<izenelib::util::UString> outRawSummaryList;
    std::vector<izenelib::util::UString> outFullTextList;

    BOOST_CHECK( documentManager->getRawTextOfDocuments(
            docIdList, "Content", true, 10, O_SNIPPET, queryTermString,
            outSnippetList, outRawSummaryList, outFullTextList ) == false);

    std::size_t docNum = docIdList.size();
    BOOST_CHECK_EQUAL(outSnippetList.size(), docNum);
    BOOST_CHECK_EQUAL(outRawSummaryList.size(), docNum);
    BOOST_CHECK_EQUAL(outFullTextList.size(), docNum);
}

BOOST_AUTO_TEST_SUITE_END()
