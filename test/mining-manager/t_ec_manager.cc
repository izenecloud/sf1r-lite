#include <mining-manager/ec-submanager/ec_manager.h>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
using namespace sf1r;
using namespace sf1r::ec;

BOOST_AUTO_TEST_SUITE(EcManager_test)

BOOST_AUTO_TEST_CASE(insert_test)
{
    std::string container = "./ec_manager_test";
    boost::filesystem::remove_all(container);
    EcManager ec_manager(container);
    BOOST_CHECK( ec_manager.Open()==true );
    std::vector<uint32_t> docid_list(2);
    docid_list[0] = 1;
    docid_list[1] = 2;
    BOOST_CHECK( ec_manager.AddDocInGroup(1, docid_list)==false );
    docid_list.clear();
    std::vector<uint32_t> candidate_list(2);
    candidate_list[0] = 4;
    candidate_list[1] = 5;
    uint32_t groupid = 0;
    BOOST_CHECK( ec_manager.AddNewGroup(docid_list, candidate_list, groupid)==true );
    BOOST_CHECK( groupid==1 );
    
    docid_list.clear();
    candidate_list[0] = 14;
    candidate_list[1] = 15;
    BOOST_CHECK( ec_manager.AddNewGroup(docid_list, candidate_list, groupid)==true );
    BOOST_CHECK( groupid==2 );
    
    docid_list.clear();
    candidate_list.clear();
    BOOST_CHECK( ec_manager.GetGroupAll(1, docid_list, candidate_list) == true );
    BOOST_CHECK( docid_list.size()==0 );
    BOOST_CHECK( candidate_list.size()==2 );
    BOOST_CHECK( candidate_list[0] == 4 );
    BOOST_CHECK( candidate_list[1] == 5 );
    docid_list.resize(2);
    docid_list[0] = 1;
    docid_list[1] = 2;
    BOOST_CHECK( ec_manager.AddDocInGroup(1, docid_list)==true );
    BOOST_CHECK( ec_manager.AddDocInGroup(1, docid_list)==false );
    BOOST_CHECK( ec_manager.AddDocInGroup(2, docid_list)==false );
    docid_list[0] = 4;
    docid_list[1] = 5;
    BOOST_CHECK( ec_manager.AddDocInGroup(2, docid_list)==true );
    docid_list.clear();
    candidate_list.clear();
    BOOST_CHECK( ec_manager.GetGroupAll(1, docid_list, candidate_list) == true );
    BOOST_CHECK( docid_list.size()==2 );
    BOOST_CHECK( candidate_list.size()==0 );
    BOOST_CHECK( docid_list[0] == 1 );
    BOOST_CHECK( docid_list[1] == 2 );
    docid_list.clear();
    candidate_list.clear();
    BOOST_CHECK( ec_manager.GetGroupAll(2, docid_list, candidate_list) == true );
    BOOST_CHECK( docid_list.size()==2 );
    BOOST_CHECK( candidate_list.size()==0 );
    BOOST_CHECK( docid_list[0] == 4 );
    BOOST_CHECK( docid_list[1] == 5 );
    
}

BOOST_AUTO_TEST_CASE(remove_test)
{
    std::string container = "./ec_manager_test";
    boost::filesystem::remove_all(container);
    EcManager ec_manager(container);
    BOOST_CHECK( ec_manager.Open()==true );
    std::vector<uint32_t> docid_list(2);
    std::vector<uint32_t> candidate_list(2);
    docid_list[0] = 1;
    docid_list[1] = 2;
    candidate_list[0] = 4;
    candidate_list[1] = 5;
    uint32_t groupid = 0;
    BOOST_CHECK( ec_manager.AddNewGroup(docid_list, candidate_list, groupid)==true );
    BOOST_CHECK( groupid==1 );
    docid_list[0] = 4;
    docid_list[1] = 5;
    candidate_list.clear();
    candidate_list.resize(1, 6);
    BOOST_CHECK( ec_manager.AddNewGroup(docid_list, candidate_list, groupid)==true );
    BOOST_CHECK( groupid==2 );
    
    docid_list.clear();
    docid_list.resize(1, 1);
    BOOST_CHECK( ec_manager.RemoveDocInGroup(1, docid_list)==true );
    
    docid_list.clear();
    candidate_list.clear();
    BOOST_CHECK( ec_manager.GetGroupAll(1, docid_list, candidate_list) == true );
    BOOST_CHECK( docid_list.size()==1 );
    BOOST_CHECK( candidate_list.size()==0 );
    BOOST_CHECK( docid_list[0] == 2 );
    
    docid_list.clear();
    candidate_list.clear();
    BOOST_CHECK( ec_manager.GetGroupAll(2, docid_list, candidate_list) == true );
    BOOST_CHECK( docid_list.size()==2 );
    BOOST_CHECK( candidate_list.size()==1 );
    BOOST_CHECK( docid_list[0] == 4 );
    BOOST_CHECK( docid_list[1] == 5 );
    BOOST_CHECK( candidate_list[0] == 6 );
    
    docid_list.clear();
    candidate_list.clear();
    BOOST_CHECK( ec_manager.GetGroupAll(3, docid_list, candidate_list) == true );
    BOOST_CHECK( docid_list.size()==0 );
    BOOST_CHECK( candidate_list.size()==1 );
    BOOST_CHECK( candidate_list[0] == 1 );
 
    
}

BOOST_AUTO_TEST_CASE(delete_test)
{
    std::string container = "./ec_manager_test";
    boost::filesystem::remove_all(container);
    EcManager ec_manager(container);
    BOOST_CHECK( ec_manager.Open()==true );
    std::vector<uint32_t> docid_list(2);
    std::vector<uint32_t> candidate_list(2);
    docid_list[0] = 1;
    docid_list[1] = 2;
    candidate_list[0] = 4;
    candidate_list[1] = 5;
    uint32_t groupid = 0;
    BOOST_CHECK( ec_manager.AddNewGroup(docid_list, candidate_list, groupid)==true );
    BOOST_CHECK( groupid==1 );
    docid_list[0] = 4;
    docid_list[1] = 5;
    candidate_list.clear();
    candidate_list.resize(1, 6);
    BOOST_CHECK( ec_manager.AddNewGroup(docid_list, candidate_list, groupid)==true );
    BOOST_CHECK( groupid==2 );
    
    docid_list.clear();
    docid_list.resize(1, 1);
    BOOST_CHECK( ec_manager.RemoveDocInGroup(1, docid_list)==true );
    BOOST_CHECK( ec_manager.DeleteDoc(1)==true );
    BOOST_CHECK( ec_manager.DeleteDoc(2)==true );
    docid_list.clear();
    candidate_list.clear();
    BOOST_CHECK( ec_manager.GetGroupAll(1, docid_list, candidate_list) == false );
    
    docid_list.clear();
    candidate_list.clear();
    BOOST_CHECK( ec_manager.GetGroupAll(2, docid_list, candidate_list) == true );
    BOOST_CHECK( docid_list.size()==2 );
    BOOST_CHECK( candidate_list.size()==1 );
    BOOST_CHECK( docid_list[0] == 4 );
    BOOST_CHECK( docid_list[1] == 5 );
    BOOST_CHECK( candidate_list[0] == 6 );
    
    docid_list.clear();
    candidate_list.clear();
    BOOST_CHECK( ec_manager.GetGroupAll(3, docid_list, candidate_list) == false );
 
    
}

BOOST_AUTO_TEST_CASE(serialize_test)
{
    std::string container = "./ec_manager_test";
    boost::filesystem::remove_all(container);
    std::vector<uint32_t> docid_list(2);
    std::vector<uint32_t> candidate_list(2);
    {
        EcManager ec_manager(container);
        BOOST_CHECK( ec_manager.Open()==true );
        
        docid_list[0] = 1;
        docid_list[1] = 2;
        candidate_list[0] = 4;
        candidate_list[1] = 5;
        uint32_t groupid = 0;
        BOOST_CHECK( ec_manager.AddNewGroup(docid_list, candidate_list, groupid)==true );
        BOOST_CHECK( groupid==1 );
        docid_list[0] = 4;
        docid_list[1] = 5;
        candidate_list.clear();
        candidate_list.resize(1, 6);
        BOOST_CHECK( ec_manager.AddNewGroup(docid_list, candidate_list, groupid)==true );
        BOOST_CHECK( groupid==2 );
        
        docid_list.clear();
        docid_list.resize(1, 1);
        BOOST_CHECK( ec_manager.RemoveDocInGroup(1, docid_list)==true );
        BOOST_CHECK( ec_manager.Flush() == true );
    }
    
    {
        EcManager ec_manager(container);
        BOOST_CHECK( ec_manager.Open()==true );
        docid_list.clear();
        candidate_list.clear();
        BOOST_CHECK( ec_manager.GetGroupAll(1, docid_list, candidate_list) == true );
        BOOST_CHECK( docid_list.size()==1 );
        BOOST_CHECK( candidate_list.size()==0 );
        BOOST_CHECK( docid_list[0] == 2 );
        
        docid_list.clear();
        candidate_list.clear();
        BOOST_CHECK( ec_manager.GetGroupAll(2, docid_list, candidate_list) == true );
        BOOST_CHECK( docid_list.size()==2 );
        BOOST_CHECK( candidate_list.size()==1 );
        BOOST_CHECK( docid_list[0] == 4 );
        BOOST_CHECK( docid_list[1] == 5 );
        BOOST_CHECK( candidate_list[0] == 6 );
        
        docid_list.clear();
        candidate_list.clear();
        BOOST_CHECK( ec_manager.GetGroupAll(3, docid_list, candidate_list) == true );
        BOOST_CHECK( docid_list.size()==0 );
        BOOST_CHECK( candidate_list.size()==1 );
        BOOST_CHECK( candidate_list[0] == 1 );
    }
    
}

BOOST_AUTO_TEST_SUITE_END() 
