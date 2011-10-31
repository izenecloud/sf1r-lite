#include <product-manager/simple_data_source.h>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
using namespace sf1r;

BOOST_AUTO_TEST_SUITE(ProductManager_test)


void CheckUuid(SimpleDataSource* data_source, const std::string& suuid, uint32_t exceptid, const std::vector<uint32_t>& result)
{
    std::vector<uint32_t> cresult(result);
    std::sort(cresult.begin(), cresult.end());
    
    izenelib::util::UString uuid(suuid, izenelib::util::UString::UTF_8);
    std::vector<uint32_t> gresult;
    data_source->GetDocIdList(uuid, gresult, exceptid);
    std::sort(gresult.begin(), gresult.end());
    BOOST_CHECK( cresult.size() == gresult.size() );
    for(uint32_t i=0;i<cresult.size();i++)
    {
        BOOST_CHECK( cresult[i] == gresult[i]);
    }
}

BOOST_AUTO_TEST_CASE(normal_test)
{
    //test GetDocIdList and UpdateUuid
    std::vector<PMDocumentType>* document_list = new std::vector<PMDocumentType>();
    PMConfig pm_config = PMConfig::GetDefaultPMConfig();
    std::vector<std::string> suuid_list(10);
    for(uint32_t docid=1; docid<=10; docid++)
    {
        std::string suuid;
        if(docid==1)
        {
            suuid = "uuid-1";
        }
        else if(docid==2)
        {
            suuid = "uuid-2";
        }
        else
        {
            suuid = "uuid-3";
        }
        suuid_list[docid-1] = suuid;
    }
    
    
    for(uint32_t docid=1; docid<=10; docid++)
    {
        PMDocumentType doc;
        doc.setId(docid);
        doc.property(pm_config.uuid_property_name) = izenelib::util::UString(suuid_list[docid-1], izenelib::util::UString::UTF_8);
        document_list->push_back(doc);
    }
    SimpleDataSource data_source(pm_config, document_list);
    {
        std::vector<uint32_t> result;
        CheckUuid(&data_source, "uuid-1", 1, result);
        result.push_back(1);
        CheckUuid(&data_source, "uuid-1", 2, result);
    }
    {
        std::vector<uint32_t> result;
        for(uint32_t i=3;i<=10;i++)
        {
            result.push_back(i);
        }
        CheckUuid(&data_source, "uuid-3", 1, result);
        result.erase( std::remove(result.begin(), result.end(), (uint32_t)4),result.end());
        CheckUuid(&data_source, "uuid-3", 4, result);
    }
    
    {
        std::vector<uint32_t> docid_list(3);
        docid_list[0] = 2;
        docid_list[1] = 3;
        docid_list[2] = 4;
        izenelib::util::UString uuid = izenelib::util::UString("uuid-x", izenelib::util::UString::UTF_8);
        BOOST_CHECK( data_source.UpdateUuid( docid_list, uuid) );
    }
    
    {
        izenelib::util::UString uuid = izenelib::util::UString("uuid-1", izenelib::util::UString::UTF_8);
        std::vector<uint32_t> result(1,1);
        CheckUuid(&data_source, "uuid-1", 100, result);
        result.clear();
        CheckUuid(&data_source, "uuid-2", 100, result);
        result.clear();
        for(uint32_t i=5;i<=10;i++)
        {
            result.push_back(i);
        }
        CheckUuid(&data_source, "uuid-3", 100, result);
        result.clear();
        for(uint32_t i=2;i<=4;i++)
        {
            result.push_back(i);
        }
        CheckUuid(&data_source, "uuid-x", 100, result);
    }
    
    
    delete document_list;
    
}


BOOST_AUTO_TEST_SUITE_END() 
