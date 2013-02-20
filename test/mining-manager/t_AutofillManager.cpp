///
/// @file t_AutofillManager.cpp
/// @brief test autofill works
/// @author Hongliang Zhao <hongliang.zhao@b5m.com>
/// @date Created 2013-02-05
///
#include <mining-manager/auto-fill-submanager/AutoFillSubManager.h>
#include <mining-manager/auto-fill-submanager/AutoFillChildManager.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <log-manager/UserQuery.h>
#include <log-manager/LogAnalysis.h>
using namespace sf1r;
using namespace std;
using namespace boost;
namespace sf1r
{

void prepareQueryData_Update(std::vector<UserQuery> & QueryList)
{
    std::vector<string> rawQueryList;
    rawQueryList.push_back("ipad 3");
    rawQueryList.push_back("ipad 2");
    rawQueryList.push_back("ipad 2 16g");
    rawQueryList.push_back("iphone");
    rawQueryList.push_back("iphone4s");

    rawQueryList.push_back("  jia   fang  ");
    rawQueryList.push_back("jeep  , ");
    rawQueryList.push_back("，iphone   ");
    rawQueryList.push_back("jiafang|jianpan");

    unsigned int queryCount = 0;
    for (std::vector<string>::iterator i = rawQueryList.begin(); i != rawQueryList.end(); ++i)
    {
        ++queryCount;
        UserQuery userQuery;
        userQuery.setHitDocsNum(queryCount);
        userQuery.setCount(queryCount);
        userQuery.setQuery(*i);

        QueryList.push_back(userQuery);
    }
}

void prepareQueryData(std::vector<UserQuery> & QueryList)
{
    std::vector<string> rawQueryList;
    rawQueryList.push_back("iphone");
    rawQueryList.push_back("iphone5");
    rawQueryList.push_back("iphone4s");
    rawQueryList.push_back("iphone4s8g");
    rawQueryList.push_back("ipad");
    rawQueryList.push_back("jiezhi");
    rawQueryList.push_back("jiake");
    rawQueryList.push_back("jiafa");;
    rawQueryList.push_back("jiumuwang");
    rawQueryList.push_back("jianpan");
    rawQueryList.push_back("jeep");
    rawQueryList.push_back("jiafang1");

    rawQueryList.push_back("jiafang|jianpan");
    rawQueryList.push_back("  jia   fang  ");
    rawQueryList.push_back("jeep  , ");
    rawQueryList.push_back("，iphone   ");
    
    unsigned int queryCount = 0;
    for (std::vector<string>::iterator i = rawQueryList.begin(); i != rawQueryList.end(); ++i)
    {
        ++queryCount;
        UserQuery userQuery;
        userQuery.setHitDocsNum(queryCount);
        userQuery.setCount(queryCount);
        userQuery.setQuery(*i);

        QueryList.push_back(userQuery);
    }
}

void createAutoFillDic()
{
    if (!boost::filesystem::is_directory("./querydata"))
    {
        boost::filesystem::create_directories("./querydata");
    }
    if (!boost::filesystem::is_directory("./scd"))
    {
        boost::filesystem::create_directories("./scd");
    }
}

void removeAutoFillDic()
{
    if (boost::filesystem::is_directory("./querydata"))
    {
        boost::filesystem::remove_all("./querydata");
    }
    if (boost::filesystem::is_directory("./scd"))
    {
        boost::filesystem::remove_all("./scd");
    }
}

void clearAutoFillDic()
{
    boost::filesystem::path path1("./querydata");
    boost::filesystem::remove_all(path1);

    boost::filesystem::path path2("./scd");
    boost::filesystem::remove_all(path2);
}


class AutoFillTestFixture
{
private:
    AutoFillChildManager* autofillManager_;
public:
    AutoFillTestFixture()
    {
        autofillManager_ = new AutoFillChildManager(false);
    }
    ~AutoFillTestFixture()
    {
        if (autofillManager_ != NULL)
        {
            delete autofillManager_;
            autofillManager_ = NULL;
        }
    }

    void buildNewAutoFill()
    {
        stopAutofill();
        autofillManager_ = new AutoFillChildManager(false);
    }

    void checkAutofillUpdateResult()
    {
        std::vector<std::pair<izenelib::util::UString, uint32_t> > ResultList;
        std::string topString;
        std::string bottomString;
        std::string prefix = "i";
        izenelib::util::UString Uquery(prefix, izenelib::util::UString::UTF_8);
        ResultList.clear();
        autofillManager_->getAutoFillList(Uquery, ResultList);
        BOOST_CHECK_EQUAL(ResultList.size(), 8U);
        cout<<"TEST 1"<<endl;
        izenelib::util::UString USTR;
        USTR = ResultList[0].first;
        USTR.convertString(topString, izenelib::util::UString::UTF_8);
        USTR = ResultList[ResultList.size() - 1].first;
        USTR.convertString(bottomString, izenelib::util::UString::UTF_8);
        BOOST_CHECK_EQUAL(topString, "iphone4s");
        BOOST_CHECK_EQUAL(bottomString, "ipad 3");

        prefix = "j";
        ResultList.clear();
        cout<<"TEST 2"<<endl;
        izenelib::util::UString Uquery1(prefix, izenelib::util::UString::UTF_8);
        autofillManager_->getAutoFillList(Uquery1, ResultList);
        BOOST_CHECK_EQUAL(ResultList.size(), 8U);
        USTR = ResultList[0].first;
        USTR.convertString(topString, izenelib::util::UString::UTF_8);
        USTR = ResultList[ResultList.size() - 1].first;
        USTR.convertString(bottomString, izenelib::util::UString::UTF_8);
        BOOST_CHECK_EQUAL(topString, "jia fang");
        BOOST_CHECK_EQUAL(bottomString, "jiezhi");

        prefix = "jia";
        ResultList.clear();
        cout<<"TEST 3"<<endl;
        izenelib::util::UString Uquery2(prefix, izenelib::util::UString::UTF_8);
        autofillManager_->getAutoFillList(Uquery2, ResultList);
        BOOST_CHECK_EQUAL(ResultList.size(), 5U);
        USTR = ResultList[0].first;
        USTR.convertString(topString, izenelib::util::UString::UTF_8);
        USTR = ResultList[ResultList.size() - 1].first;
        USTR.convertString(bottomString, izenelib::util::UString::UTF_8);
        BOOST_CHECK_EQUAL(topString, "jia fang");
        BOOST_CHECK_EQUAL(bottomString, "jiake");
    }

    void checkAutofillResult()
    {
        std::vector<std::pair<izenelib::util::UString, uint32_t> > ResultList;
        std::string topString;
        std::string bottomString;
        std::string prefix = "i";
        izenelib::util::UString Uquery(prefix, izenelib::util::UString::UTF_8);
        ResultList.clear();
        autofillManager_->getAutoFillList(Uquery, ResultList);
        BOOST_CHECK_EQUAL(ResultList.size(), 5U);
        cout<<"TEST 1"<<endl;
        izenelib::util::UString USTR;
        USTR = ResultList[0].first;
        USTR.convertString(topString, izenelib::util::UString::UTF_8);
        USTR = ResultList[ResultList.size() - 1].first;
        USTR.convertString(bottomString, izenelib::util::UString::UTF_8);
        BOOST_CHECK_EQUAL(topString, "ipad");
        BOOST_CHECK_EQUAL(bottomString, "iphone");

        prefix = "j";
        ResultList.clear();
        cout<<"TEST 2"<<endl;
        izenelib::util::UString Uquery1(prefix, izenelib::util::UString::UTF_8);
        autofillManager_->getAutoFillList(Uquery1, ResultList);
        BOOST_CHECK_EQUAL(ResultList.size(), 8U);
        USTR = ResultList[0].first;
        USTR.convertString(topString, izenelib::util::UString::UTF_8);
        USTR = ResultList[ResultList.size() - 1].first;
        USTR.convertString(bottomString, izenelib::util::UString::UTF_8);
        BOOST_CHECK_EQUAL(topString, "jia fang");
        BOOST_CHECK_EQUAL(bottomString, "jiezhi");

        prefix = "jia";
        ResultList.clear();
        cout<<"TEST 3"<<endl;
        izenelib::util::UString Uquery2(prefix, izenelib::util::UString::UTF_8);
        autofillManager_->getAutoFillList(Uquery2, ResultList);
        BOOST_CHECK_EQUAL(ResultList.size(), 5U);
        USTR = ResultList[0].first;
        USTR.convertString(topString, izenelib::util::UString::UTF_8);
        USTR = ResultList[ResultList.size() - 1].first;
        USTR.convertString(bottomString, izenelib::util::UString::UTF_8);
        BOOST_CHECK_EQUAL(topString, "jia fang");
        BOOST_CHECK_EQUAL(bottomString, "jiake");
    }

    void testInitAutoFill()
    {
        createAutoFillDic();
        CollectionPath collectionPath_;
        collectionPath_.setBasePath("./");
        collectionPath_.setQueryDataPath("./querydata");
        collectionPath_.setScdPath("/scd");

        std::string collection_name_ = "b5m_test";
        std::string cronName_ = "30 2 * * *";

        std::vector<UserQuery> QueryList;
        prepareQueryData(QueryList);
        std::string instance = "b5mp";
        bool isFromLevelDB = false;
        BOOST_CHECK_EQUAL(autofillManager_->Init_ForTest(collectionPath_, collection_name_, cronName_, instance, isFromLevelDB, QueryList), true);
        
        cout << "Begin test init autofill..............................."<<endl;
        checkAutofillResult();
    }

    void testStartAutoFillFromLevelDB(bool isupdate)
    {
        CollectionPath collectionPath_;
        collectionPath_.setBasePath("./");
        collectionPath_.setQueryDataPath("./querydata");
        collectionPath_.setScdPath("/scd");

        std::string collection_name_ = "b5m_test";
        std::string cronName_ = "30 2 * * *";

        std::vector<UserQuery> QueryList;
        prepareQueryData(QueryList);
        std::string instance = "b5mp";
        bool isFromLevelDB = true;
        BOOST_CHECK_EQUAL(autofillManager_->Init_ForTest(collectionPath_, collection_name_, cronName_, instance, isFromLevelDB, QueryList), true);
        
        cout << "Begin testStartAutoFillFromLevelDB ...................."<<endl;
        if (!isupdate)
        {
            checkAutofillResult();
        }
        else
            checkAutofillUpdateResult();

    }

    void testUpdateAutofill()
    {
        std::vector<UserQuery> QueryList;
        prepareQueryData_Update(QueryList);
        autofillManager_->updateFromLog_ForTest(QueryList);

        cout << "Begin update AutoFill ................................."<<endl;
        checkAutofillUpdateResult();
    }

    void stopAutofill()
    {
        if (autofillManager_ != NULL)
        {
            delete autofillManager_;
            autofillManager_ = NULL;
        }
    }
};


BOOST_AUTO_TEST_SUITE(AutoFill)
BOOST_FIXTURE_TEST_CASE(checkAutofill, AutoFillTestFixture)
{
    clearAutoFillDic();
    bool isupdate = false;
    BOOST_TEST_MESSAGE("[Test init AutoFill]");
    testInitAutoFill();
    stopAutofill();

    BOOST_TEST_MESSAGE("[Test restart AutoFill]");
    buildNewAutoFill();
    testStartAutoFillFromLevelDB(isupdate);

    BOOST_TEST_MESSAGE("[Test update AutoFill]");
    testUpdateAutofill();
    stopAutofill();
    isupdate = true;

    BOOST_TEST_MESSAGE("[Test update AutoFill]");
    buildNewAutoFill();
    testStartAutoFillFromLevelDB(isupdate);
    stopAutofill();
    clearAutoFillDic();
}
BOOST_AUTO_TEST_SUITE_END()
}
