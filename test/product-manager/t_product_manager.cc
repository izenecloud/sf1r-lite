#include <product-manager/simple_data_source.h>
#include <product-manager/simple_operation_processor.h>
#include <product-manager/product_manager.h>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
using namespace sf1r;

BOOST_AUTO_TEST_SUITE(ProductManager_test)

static PMConfig pm_config = PMConfig::GetDefaultPMConfig();

void SetDocProperty(PMDocumentType& doc, const std::string& name, const std::string& value)
{
    doc.property(name) = izenelib::util::UString(value, izenelib::util::UString::UTF_8);
}

void InitDoc(PMDocumentType& doc, const std::string& docid, const std::string& price)
{
    SetDocProperty(doc, pm_config.docid_property_name, docid);
    SetDocProperty(doc, pm_config.price_property_name, price);
}

void InitInsertDocs(std::vector<PMDocumentType>& docs)
{
    {
        PMDocumentType doc;
        InitDoc(doc, "doc1", "1.0");
        docs.push_back(doc);
    }
    {
        PMDocumentType doc;
        InitDoc(doc, "doc2", "2.0");
        docs.push_back(doc);
    }
    {
        PMDocumentType doc;
        InitDoc(doc, "doc3", "3.0-4.0");
        docs.push_back(doc);
    }
    {
        PMDocumentType doc;
        InitDoc(doc, "doc4", "1.5~3.5");
        docs.push_back(doc);
    }
}

void CheckInsert(std::vector<PMDocumentType>* document_list, std::vector<std::pair<int, PMDocumentType> >& output)
{
    BOOST_CHECK( output.size() == 4);
    BOOST_CHECK( document_list->size() == output.size());
    boost::unordered_set<std::string> index;
    for(uint32_t i=0;i<document_list->size();i++)
    {
        BOOST_CHECK( output[i].first == 1 );
        const PMDocumentType& doc = (*document_list)[i];
        PMDocumentType::property_const_iterator it = doc.findProperty(pm_config.uuid_property_name);
        BOOST_CHECK( it != doc.propertyEnd());
        izenelib::util::UString uuid = it->second.get<izenelib::util::UString>();
        std::string suuid;
        uuid.convertString(suuid, izenelib::util::UString::UTF_8);
        boost::unordered_set<std::string>::iterator uit = index.find(suuid);
        BOOST_CHECK( uit == index.end());
        index.insert(suuid);
        {
            const PMDocumentType& odoc = output[i].second;
            PMDocumentType::property_const_iterator oit = odoc.findProperty(pm_config.docid_property_name);
            BOOST_CHECK( oit != odoc.propertyEnd());
            izenelib::util::UString odocid = oit->second.get<izenelib::util::UString>();
            BOOST_CHECK(uuid==odocid);
        }
    }
}



BOOST_AUTO_TEST_CASE(insert_test)
{
    std::vector<PMDocumentType>* document_list = new std::vector<PMDocumentType>();
    SimpleDataSource* data_source = new SimpleDataSource(pm_config, document_list);
    SimpleOperationProcessor* op_processor = new SimpleOperationProcessor();
    ProductManager pm(data_source, op_processor, pm_config);
    
    
    std::vector<PMDocumentType> docs;
    InitInsertDocs(docs);
    for(uint32_t i=0;i<docs.size();i++)
    {
        izenelib::ir::indexmanager::IndexerDocument index_document;
        PMDocumentType& doc = docs[i];
        doc.setId(i+1);
        BOOST_CHECK( pm.HookInsert(doc, index_document) );
        document_list->push_back(doc);
    }
    BOOST_CHECK( op_processor->Finish() );
    BOOST_CHECK( document_list->size() == docs.size() );
    CheckInsert(document_list, op_processor->Data());
    delete op_processor;
    delete data_source;
    delete document_list;
    
}

BOOST_AUTO_TEST_CASE(simple_update_test)
{
    std::vector<PMDocumentType>* document_list = new std::vector<PMDocumentType>();
    SimpleDataSource* data_source = new SimpleDataSource(pm_config, document_list);
    SimpleOperationProcessor* op_processor = new SimpleOperationProcessor();
    ProductManager pm(data_source, op_processor, pm_config);
    
    
    std::vector<PMDocumentType> insert_docs;
    InitInsertDocs(insert_docs);
    for(uint32_t i=0;i<insert_docs.size();i++)
    {
        izenelib::ir::indexmanager::IndexerDocument index_document;
        PMDocumentType& doc = insert_docs[i];
        doc.setId(i+1);
        BOOST_CHECK( pm.HookInsert(doc, index_document) );
        document_list->push_back(doc);
    }
    std::vector<izenelib::util::UString> uuid_list(document_list->size());
    for(uint32_t i=0;i<uuid_list.size();i++)
    {
        const PMDocumentType& doc = (*document_list)[i];
        PMDocumentType::property_const_iterator it = doc.findProperty(pm_config.uuid_property_name);
        BOOST_CHECK( it != doc.propertyEnd());
        uuid_list[i] = it->second.get<izenelib::util::UString>();
    }
    
    {
        PMDocumentType doc;
        InitDoc(doc, "doc2", "4.0-10.0");
        doc.setId(2);
        izenelib::ir::indexmanager::IndexerDocument index_document;
        index_document.setId(2);
        BOOST_CHECK( pm.HookUpdate(doc, index_document, false) );
        (*document_list)[1] = doc;
    }
    {
        PMDocumentType doc;
        InitDoc(doc, "doc1", "5.0");
        doc.setId(1);
        izenelib::ir::indexmanager::IndexerDocument index_document;
        index_document.setId(1);
        BOOST_CHECK( pm.HookUpdate(doc, index_document, true) );
        (*document_list)[0] = doc;
    }
    BOOST_CHECK( op_processor->Finish() );
    std::vector<std::pair<int, PMDocumentType> >& data = op_processor->Data();
    BOOST_CHECK( data.size()==6 );
    for(uint32_t i=0;i<4;i++)
    {
        BOOST_CHECK(data[i].first==1);
    }
    for(uint32_t i=4;i<6;i++)
    {
        BOOST_CHECK(data[i].first==2);
    }
    BOOST_CHECK( data[0].second.property(pm_config.docid_property_name) ==
    data[5].second.property(pm_config.docid_property_name) );
    BOOST_CHECK( data[1].second.property(pm_config.docid_property_name) ==
    data[4].second.property(pm_config.docid_property_name) );
    
    delete op_processor;
    delete data_source;
    delete document_list;
    
}

BOOST_AUTO_TEST_CASE(simple_delete_test)
{
    std::vector<PMDocumentType>* document_list = new std::vector<PMDocumentType>();
    SimpleDataSource* data_source = new SimpleDataSource(pm_config, document_list);
    SimpleOperationProcessor* op_processor = new SimpleOperationProcessor();
    ProductManager pm(data_source, op_processor, pm_config);
    
    
    std::vector<PMDocumentType> insert_docs;
    InitInsertDocs(insert_docs);
    for(uint32_t i=0;i<insert_docs.size();i++)
    {
        izenelib::ir::indexmanager::IndexerDocument index_document;
        PMDocumentType& doc = insert_docs[i];
        BOOST_CHECK( pm.HookInsert(doc, index_document) );
        document_list->push_back(doc);
    }
    std::vector<izenelib::util::UString> uuid_list(document_list->size());
    for(uint32_t i=0;i<uuid_list.size();i++)
    {
        const PMDocumentType& doc = (*document_list)[i];
        PMDocumentType::property_const_iterator it = doc.findProperty(pm_config.uuid_property_name);
        BOOST_CHECK( it != doc.propertyEnd());
        uuid_list[i] = it->second.get<izenelib::util::UString>();
    }
    
    {
        BOOST_CHECK( pm.HookDelete(2) );
    }
    BOOST_CHECK( op_processor->Finish() );
    std::vector<std::pair<int, PMDocumentType> >& data = op_processor->Data();
    BOOST_CHECK( data.size()==5 );
    for(uint32_t i=0;i<4;i++)
    {
        BOOST_CHECK(data[i].first==1);
    }
    for(uint32_t i=4;i<5;i++)
    {
        BOOST_CHECK(data[i].first==3);
    }
    BOOST_CHECK( data[1].second.property(pm_config.docid_property_name) ==
    data[4].second.property(pm_config.docid_property_name) );
    
    delete op_processor;
    delete data_source;
    delete document_list;
    
}




BOOST_AUTO_TEST_SUITE_END() 

