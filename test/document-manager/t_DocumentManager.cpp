#include <document-manager/text-summarization-submanager/TextSummarizationSubManager.h>
#include <document-manager/DocumentManager.h>
#include <document-manager/Document.h>

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
}

void makeSchema(std::set<PropertyConfig, PropertyComp>& propertyConfig)
{
    PropertyConfig pconfig1;
    pconfig1.setName("DOCID");
    PropertyConfig pconfig2;
    pconfig2.setName("DATE");
    PropertyConfig pconfig3;
    pconfig3.setName("Title");
    PropertyConfig pconfig4;
    pconfig4.setName("Content");
    PropertyConfig pconfig5;
    pconfig5.setName("ImgURL");

    propertyConfig.insert(pconfig1);
    propertyConfig.insert(pconfig2);
    propertyConfig.insert(pconfig3);
    propertyConfig.insert(pconfig3);
    propertyConfig.insert(pconfig5);
    propertyConfig.insert(pconfig4);
    propertyConfig.insert(pconfig5);
}


boost::shared_ptr<DocumentManager>
createDocumentManager()
{
    bfs::path dmPath(bfs::path(".") /"document/");
    bfs::create_directories(dmPath);	
    std::set<PropertyConfig, PropertyComp> propertyConfig;
    makeSchema(propertyConfig);

    boost::shared_ptr<DocumentManager> ret(
        new DocumentManager(
            dmPath.string(),
            propertyConfig,
            encoding,
            2000
        )
    );
    return ret;
}

void buildProperty(izenelib::util::UString& string, int iLength)
{
    for( int i = 0 ; i < iLength ; ++i )
        string += alpha[r()];
}

void prepareDocument(unsigned int docId, Document& document)
{
    document.setId(docId);
    izenelib::util::UString property;
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
    izenelib::util::UString property;
    buildProperty(property, 5);
    document.property("DOCID") = property;
    property.clear();
    buildProperty(property, 10);
    document.property("DATE") = property;
    //property.clear();
    //buildProperty(property, 100);
    document.property("Title") = value;
    //property.clear();
    //buildProperty(property, 2000);
    document.property("Content") = value + value;
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
    for(unsigned int i = 1; i <= 1000; ++i)
    {
        Document document;
        prepareDocument(i, document);
        BOOST_CHECK_EQUAL(documentManager->insertDocument(document),true);
    }

    for(unsigned int i = 1; i <= 1000; ++i)
    {
        BOOST_CHECK_EQUAL(documentManager->removeDocument(i),true);
    }

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
    queryTermString.push_back( izenelib::util::UString( "a" ) );

    std::vector<izenelib::util::UString> outSnippetList;
    std::vector<izenelib::util::UString> outRawSummaryList;
    std::vector<izenelib::util::UString> outFullTextList;

    bool expectedValue = false;

    BOOST_CHECK_EQUAL( documentManager->getRawTextOfDocuments(
            docIdList, "Content", true, 10, O_SNIPPET, queryTermString,
            outSnippetList, outRawSummaryList, outFullTextList ) ,expectedValue );
    BOOST_CHECK_EQUAL( outSnippetList.empty(), !expectedValue );
    BOOST_CHECK_EQUAL( outRawSummaryList.empty(), !expectedValue );
    BOOST_CHECK_EQUAL( outFullTextList.empty(), !expectedValue );
}



BOOST_AUTO_TEST_SUITE_END() 

