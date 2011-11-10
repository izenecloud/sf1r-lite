///
/// @file   t_QueryParser.cpp
/// @author Dohyun Yun
/// @date   2010-06-23
///

#include "test_def.h"

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <query-manager/QueryParser.h>
#include <common/SFLogger.h>

using namespace std;
using namespace sf1r;
using namespace boost::unit_test;
namespace bfs = boost::filesystem;
using izenelib::ir::idmanager::IDManager;

namespace
{
const string TEST_DIR_STR = "t_QueryParser_dir";
}

struct QueryParserTestFixture
{
    std::vector<AnalysisInfo> analysisInfoList;
    boost::shared_ptr<LAManager> laManager;
    boost::shared_ptr<IDManager> idManager;

    QueryParserTestFixture()
        : laManager(new LAManager)
    {
        LAManagerConfig config;
        initConfig( analysisInfoList, config );

        bfs::create_directories(TEST_DIR_STR);
        idManager.reset(new IDManager(TEST_DIR_STR + "/idm_qmt"));

        LAPool::getInstance()->init(config);
        QueryParser::initOnlyOnce();

        // Initialize SFLogger
        sflog->init("sqlite3://" + TEST_DIR_STR + "/COBRA");

        // Build restrict term dictionary
        std::string restrictDicPath(TEST_DIR_STR + "/restrict.txt");
        ofstream fpout(restrictDicPath.c_str());
        BOOST_CHECK( fpout.is_open() );
        fpout << "news" << endl;
        fpout << "korean news" << endl;
        fpout.close();
        QueryUtility::buildRestrictTermDictionary(restrictDicPath, idManager);

    } // end - initOnlyOnce

    ~QueryParserTestFixture()
    {
        idManager->close();
        bfs::remove_all(TEST_DIR_STR);
    }
};

void keywordTreeCheck(const QueryTreePtr& queryTree, const std::string& queryStr)
{
    izenelib::util::UString queryUStr(queryStr, izenelib::util::UString::UTF_8);
    BOOST_CHECK_EQUAL( QueryTree::KEYWORD , queryTree->type_ );
    BOOST_CHECK_EQUAL( queryStr , queryTree->keyword_ );
    BOOST_CHECK_NE( 0U , queryTree->keywordId_ );
    BOOST_CHECK( queryUStr == queryTree->keywordUString_ );
    BOOST_CHECK_EQUAL( 0U , queryTree->children_.size() );
} // end - keywordTreeCheck

BOOST_FIXTURE_TEST_SUITE(QueryParser_test, QueryParserTestFixture)

BOOST_AUTO_TEST_CASE(normalizeQuery_test)
{

#define USTRING(str) UString(str, UString::UTF_8)

    using izenelib::util::UString;
    std::vector<UString> queryStringList, normResultList;

    queryStringList.push_back(USTRING("   test"));
    queryStringList.push_back(USTRING("(  hello  |   world  )"));
    queryStringList.push_back(USTRING("{  test  case } ^ 12(month case)"));
    queryStringList.push_back(USTRING("(bracket close)(open space)"));
    normResultList.push_back(USTRING("test"));
    normResultList.push_back(USTRING("(hello|world)"));
    normResultList.push_back(USTRING("{test case}^12&(month&case)"));
    normResultList.push_back(USTRING("(bracket&close)&(open&space)"));

    UString out;
    QueryParser queryParser(laManager, idManager);

    for(size_t i = 0; i < queryStringList.size(); i++)
    {
        queryParser.normalizeQuery( queryStringList[i], out, true );
        BOOST_CHECK_EQUAL( out , normResultList[i] );
    }

    UString queryString = USTRING("{  test  \"case  \"  * no?unigram } ^ 12(month \" case\" *no?unigram");
    UString normResults[2] = {USTRING("{test \"case \" * no?unigram}^12&(month&\" case\"&*no?unigram)"), USTRING("{test \"case \" * no?unigram}^12&(month&case&no&unigram)")};

    queryParser.normalizeQuery( queryString, out, true );
    BOOST_CHECK_EQUAL( out , normResults[0] );

    queryParser.normalizeQuery( queryString, out, false );
    BOOST_CHECK_EQUAL( out , normResults[1] );
}

BOOST_AUTO_TEST_CASE(parseQuery_basic_test)
{
    std::string queryStr;
    izenelib::util::UString queryUStr;
    QueryTreePtr queryTree;
    QueryParser queryParser(laManager, idManager);

    { // KEYWORD type
        queryStr = " 본케나오게하지마라. ";
        std::string compStr = "본케나오게하지마라.";
        queryUStr.assign(queryStr, izenelib::util::UString::UTF_8);
        BOOST_CHECK( queryParser.parseQuery(queryUStr, queryTree, false, true) );
        keywordTreeCheck(queryTree, compStr);
    }
    { // WILDCARD type 1
        queryStr = "본케나오게*하지마라?";
        queryUStr.assign(queryStr, izenelib::util::UString::UTF_8);
        BOOST_CHECK( queryParser.parseQuery(queryUStr, queryTree, false, true) );
        BOOST_CHECK_EQUAL( QueryTree::TRIE_WILDCARD, queryTree->type_ );
        BOOST_CHECK_EQUAL( queryStr , queryTree->keyword_ );
        BOOST_CHECK_NE( 0U , queryTree->keywordId_ );
        BOOST_CHECK( queryUStr == queryTree->keywordUString_ );
        BOOST_CHECK_EQUAL( 0U , queryTree->children_.size() );
    }
    { // WILDCARD type 2
        queryStr = "본케나오게*하지마라?";
        queryUStr.assign(queryStr, izenelib::util::UString::UTF_8);
        BOOST_CHECK( queryParser.parseQuery(queryUStr, queryTree, true, true) );
        BOOST_CHECK_EQUAL( QueryTree::UNIGRAM_WILDCARD, queryTree->type_ );
        BOOST_CHECK_EQUAL( queryStr , queryTree->keyword_ );
        BOOST_CHECK( queryUStr == queryTree->keywordUString_ );
        BOOST_CHECK_NE( 0U , queryTree->keywordId_ );
        BOOST_CHECK_EQUAL( 11U , queryTree->children_.size() );
        std::list<QueryTreePtr>::const_iterator iter = queryTree->children_.begin();

        BOOST_CHECK_EQUAL( QueryTree::KEYWORD, (*iter)->type_ );
        BOOST_CHECK_EQUAL( "본" , (*iter)->keyword_ );
        BOOST_CHECK( izenelib::util::UString("본", izenelib::util::UString::UTF_8) ==(*iter)->keywordUString_ );
        BOOST_CHECK_NE( 0U, (*iter)->keywordId_ );

        iter++;iter++;iter++;iter++;iter++;
        BOOST_CHECK_EQUAL( QueryTree::ASTERISK, (*iter)->type_ );

        iter++;iter++;
        BOOST_CHECK_EQUAL( QueryTree::KEYWORD, (*iter)->type_ );
        BOOST_CHECK_EQUAL( "지" , (*iter)->keyword_ );
        BOOST_CHECK( izenelib::util::UString("지", izenelib::util::UString::UTF_8) == (*iter)->keywordUString_ );
        BOOST_CHECK_NE( 0U, (*iter)->keywordId_ );

        iter++;iter++;iter++;
        BOOST_CHECK_EQUAL( QueryTree::QUESTION_MARK, (*iter)->type_ );
    }
    { // AND type
        queryStr = "본케나오게 하지마라";
        queryUStr.assign(queryStr, izenelib::util::UString::UTF_8);
        BOOST_CHECK( queryParser.parseQuery(queryUStr, queryTree, true, true) );
        BOOST_CHECK_EQUAL( QueryTree::AND, queryTree->type_ );
        BOOST_CHECK_EQUAL( "", queryTree->keyword_ );
        BOOST_CHECK_EQUAL( 0U , queryTree->keywordId_ );
        BOOST_CHECK_EQUAL( 2U , queryTree->children_.size() );

        std::list<QueryTreePtr>::const_iterator iter = queryTree->children_.begin();
        QueryTreePtr child = *iter;
        keywordTreeCheck(child, "본케나오게");

        child = *(++iter);
        keywordTreeCheck(child, "하지마라");
    }
    { // AND type with chinese character
//      queryStr = "가나北京 天安门abc 123";
//      queryUStr.assign(queryStr, izenelib::util::UString::UTF_8);
//      BOOST_CHECK( queryParser.parseQuery(queryUStr, queryTree, true, true) );
//      BOOST_CHECK_EQUAL( "", queryTree->keyword_ );
//      BOOST_CHECK_EQUAL( 0U , queryTree->keywordId_ );
//      BOOST_CHECK_EQUAL( 2U , queryTree->children_.size() );

//      std::list<QueryTreePtr>::const_iterator iter = queryTree->children_.begin();
//      QueryTreePtr child = *iter;
//      keywordTreeCheck(child, "가나北京天安门abc");

//      child = *(++iter);
//      keywordTreeCheck(child, "123");
    }
    { // OR type
        queryStr = "본케나오게|하지마라";
        queryUStr.assign(queryStr, izenelib::util::UString::UTF_8);
        BOOST_CHECK( queryParser.parseQuery(queryUStr, queryTree, true, true) );
        BOOST_CHECK_EQUAL( QueryTree::OR, queryTree->type_ );
        BOOST_CHECK_EQUAL( "", queryTree->keyword_ );
        BOOST_CHECK_EQUAL( 0U , queryTree->keywordId_ );
        BOOST_CHECK_EQUAL( 2U , queryTree->children_.size() );

        std::list<QueryTreePtr>::const_iterator iter = queryTree->children_.begin();
        QueryTreePtr child = *iter;
        keywordTreeCheck(child, "본케나오게");

        child = *(++iter);
        keywordTreeCheck(child, "하지마라");
    }
    { // NOT type
        queryStr = "!빙신";
        queryUStr.assign(queryStr, izenelib::util::UString::UTF_8);
        BOOST_CHECK( queryParser.parseQuery(queryUStr, queryTree, true, true) );
        BOOST_CHECK_EQUAL( QueryTree::NOT, queryTree->type_ );
        BOOST_CHECK_EQUAL( "", queryTree->keyword_ );
        BOOST_CHECK_EQUAL( 0U , queryTree->keywordId_ );
        BOOST_CHECK_EQUAL( 1U , queryTree->children_.size() );

        QueryTreePtr child = *(queryTree->children_.begin());
        keywordTreeCheck(child, "빙신");
    }
    { // EXACT type
        queryStr = "\"본케나오게 .하지마라\"";
        queryUStr.assign(queryStr, izenelib::util::UString::UTF_8);
        BOOST_CHECK( queryParser.parseQuery(queryUStr, queryTree, true, true) );
        BOOST_CHECK_EQUAL( QueryTree::EXACT, queryTree->type_ );
        BOOST_CHECK_EQUAL( "본케나오게 .하지마라", queryTree->keyword_ );
        BOOST_CHECK_NE( 0U , queryTree->keywordId_ );
        BOOST_CHECK_EQUAL( 10U , queryTree->children_.size() );

        QTIter iter = queryTree->children_.begin();
        keywordTreeCheck(*iter++, "본");
        keywordTreeCheck(*iter++, "케");
        keywordTreeCheck(*iter++, "나");
        keywordTreeCheck(*iter++, "오");
        keywordTreeCheck(*iter++, "게");
        keywordTreeCheck(*iter++, "<PH>");
        keywordTreeCheck(*iter++, "하");
        keywordTreeCheck(*iter++, "지");
        keywordTreeCheck(*iter++, "마");
        keywordTreeCheck(*iter++, "라");
    }
    { // ORDER type
        queryStr = "[본케나오게 하지마라]";
        queryUStr.assign(queryStr, izenelib::util::UString::UTF_8);
        BOOST_CHECK( queryParser.parseQuery(queryUStr, queryTree, true, true) );
        BOOST_CHECK_EQUAL( QueryTree::ORDER, queryTree->type_ );
        BOOST_CHECK_EQUAL( "본케나오게 하지마라" , queryTree->keyword_ );
        BOOST_CHECK_NE( 0U , queryTree->keywordId_ );
        BOOST_CHECK_EQUAL( 2U , queryTree->children_.size() );

        std::list<QueryTreePtr>::const_iterator iter = queryTree->children_.begin();
        QueryTreePtr child = *iter;
        keywordTreeCheck(child, "본케나오게");

        child = *(++iter);
        keywordTreeCheck(child, "하지마라");
    }
    { // NEARBY type
        queryStr = "{본케나오게 하지마라}^1982";
        queryUStr.assign(queryStr, izenelib::util::UString::UTF_8);
        BOOST_CHECK( queryParser.parseQuery(queryUStr, queryTree, true, true) );
        BOOST_CHECK_EQUAL( QueryTree::NEARBY, queryTree->type_ );
        BOOST_CHECK_EQUAL( "본케나오게 하지마라" , queryTree->keyword_ );
        BOOST_CHECK_NE( 0U , queryTree->keywordId_ );
        BOOST_CHECK_EQUAL( 2U , queryTree->children_.size() );

        std::list<QueryTreePtr>::const_iterator iter = queryTree->children_.begin();
        QueryTreePtr child = *iter;
        keywordTreeCheck(child, "본케나오게");

        child = *(++iter);
        keywordTreeCheck(child, "하지마라");

        BOOST_CHECK_EQUAL( 1982 , queryTree->distance_ );
    }
    { // NEARBY type 2
        queryStr = "{본케나오게 하지마라}";
        queryUStr.assign(queryStr, izenelib::util::UString::UTF_8);
        BOOST_CHECK( queryParser.parseQuery(queryUStr, queryTree, true, true) );
        BOOST_CHECK_EQUAL( QueryTree::NEARBY, queryTree->type_ );
        BOOST_CHECK_EQUAL( "본케나오게 하지마라" , queryTree->keyword_ );
        BOOST_CHECK_NE( 0U , queryTree->keywordId_ );
        BOOST_CHECK_EQUAL( 2U , queryTree->children_.size() );

        std::list<QueryTreePtr>::const_iterator iter = queryTree->children_.begin();
        QueryTreePtr child = *iter;
        keywordTreeCheck(child, "본케나오게");

        child = *(++iter);
        keywordTreeCheck(child, "하지마라");

        BOOST_CHECK_EQUAL( 20 , queryTree->distance_ ); // defaut value
    }
    {
        queryStr = "news";
        queryUStr.assign(queryStr, izenelib::util::UString::UTF_8);
        BOOST_CHECK( !queryParser.parseQuery(queryUStr, queryTree, true, true) );

        queryStr = "korean news";
        queryUStr.assign(queryStr, izenelib::util::UString::UTF_8);
        BOOST_CHECK( !queryParser.parseQuery(queryUStr, queryTree, true, true) );
    } // end - Restrict Term Test
}

BOOST_AUTO_TEST_CASE(parseQuery_extreme_test1)
{
    QueryParser queryParser(laManager, idManager);

    // Insert Large amount of query.
    string queryStr("회사명.대표.자본금(단위 백만원).업종.주소 順<>특7호*상명푸드(이상기.100.식품류) 방이동 51의4효성올림픽카운티1011호*새롬후디스(현성규.50.인삼제품) 서초동 1628의29*씨앤제이브라더스(안승길.50.식품류) 가락동 119의12 201호*엠디피생활건강(정인숙.500.건강식품) 문래동 3가55의7에이스테크노타워402호*해인바다(신상우.50.농수축임산물) 방이동 196의19<>유통*가나우드코리아(강상규.50.수출입) 영등포동 2가207*가백무역(최명자.50.의류) 신천동 11의9잠실한신코아오피스텔*건훈전자(황상하.300.전자부품) 서초동 1671의5대림서초리시온상가510호*경남아이앤티(이경남.300.통신기기) 역삼동 725의25포커스빌딩 2층*경한철강(김종민.50.철강) 문래동 3가54의43*그레비컴(노경선.50.신용카드조회기) 하계동 288현대2차(아) 상가지하1층전관*글로벌콘티넨탈코리아(박상호.100.재활용품자재) 역촌동 43의64*까사아이엔티(이한우.50.원단수출입) 포이동 240일영빌딩 2층*나노윈(최병영.50.정밀측정시스템) 가락동 171의11예울빌딩 3층*네트서브웨이(오정석.20.전산소모품) 역삼동 707의38테헤란오피스빌딩 1203호*다인인터컴(백광조.30.통신장비) 여의도동 11의11한서오피스텔512*다인티앤지(이광석.50.섬유) 염창동 264의26 2층*대솔아이티(최성민.50.컴퓨터주변기기) 정릉동 921의8*동 일엠디(강권.50.공산품) 양재동 23이비지니스타워504*두리농산(김광태.100.농산물) 동 작동 105*디엘가스산업(권석범.50.가스기기류) 공항동 1351*디티유어패럴(김미정.50.의류) 신당동 193의3 713호*라이프써핑(이종철.50.전자상거래) 서교동 395의166서교빌딩 602호*레몬통상(임종말.50.농수축산물) 가락동 98의7거북이빌딩 201호*레이스조이(천?牙?50.전자상거래) 양재동 20의12 112호*머리나라(.770.무역) 석촌동 2의9금보빌딩 ");
    izenelib::util::UString queryUStr(queryStr, izenelib::util::UString::UTF_8);
    QueryTreePtr queryTree;
    BOOST_CHECK( queryParser.parseQuery(queryUStr, queryTree, false, true) );
}

BOOST_AUTO_TEST_CASE(getAnalyzedQueryTree_basic_test)
{
    /*
    QueryParser queryParser(laManager, idManager);

    {
        izenelib::util::UString queryUStr("(reds|iphone4) (!빨간 자동차)", izenelib::util::UString::UTF_8);
        QueryTreePtr queryTree;

        AnalysisInfo analysisInfo;
        analysisInfo.analyzerId_ = "la_korall";

        BOOST_CHECK(queryParser.getAnalyzedQueryTree(true, analysisInfo, queryUStr, queryTree, false));

        // ROOT -> child : DFS check
        BOOST_CHECK_EQUAL(queryTree->type_ , QueryTree::AND);

        QTIter c, gc, ggc; // child, grand child, grand grand child

        c = queryTree->children_.begin();
        BOOST_CHECK_EQUAL((*c)->type_ , QueryTree::OR);

        gc = (*c)->children_.begin();
        keywordTreeCheck ((*gc) , "red"); gc++;
        BOOST_CHECK_EQUAL((*gc)->type_ , QueryTree::AND);
        ggc = (*gc)->children_.begin();
        keywordTreeCheck (*ggc, "iphon"); ggc++;
        keywordTreeCheck (*ggc , "4");

        c++;
        BOOST_CHECK_EQUAL((*c)->type_ , QueryTree::NOT);
        gc = (*c)->children_.begin();
        keywordTreeCheck (*gc , "빨간");

        c++;
        keywordTreeCheck (*c , "자동차");
    }
    { // Use Operator | as a character in special token option
        string queryStr("abc|123\\|def");

        AnalysisInfo analysisInfo;
        analysisInfo.analyzerId_ = "la_token";
        analysisInfo.tokenizerNameList_.insert("tok_allow_ex");

        izenelib::util::UString queryUStr(queryStr, izenelib::util::UString::UTF_8);
        QueryTreePtr queryTree;
        BOOST_CHECK(queryParser.getAnalyzedQueryTree(true, analysisInfo, queryUStr, queryTree, false));

        // Check Query Tree
        BOOST_CHECK_EQUAL( queryTree->type_ , QueryTree::OR );
        BOOST_CHECK_EQUAL( queryTree->children_.size() , 2 );
        QTIter childIter = queryTree->children_.begin();
        keywordTreeCheck(*childIter++, "abc");
        keywordTreeCheck(*childIter, "123|def");
    }
    */
}

BOOST_AUTO_TEST_CASE(getAnalyzedQueryTree_phrase_test)
{
    QueryParser queryParser(laManager, idManager);

    { // Order Type
        izenelib::util::UString queryUStr("[대한민국 명바기]", izenelib::util::UString::UTF_8);
        QueryTreePtr queryTree;
        QTIter childIter;

        AnalysisInfo analysisInfo;
        analysisInfo.analyzerId_ = "la_korall";

        // Check without analysis
        BOOST_CHECK( queryParser.parseQuery(queryUStr, queryTree, false, true) );
        BOOST_CHECK_EQUAL(queryTree->type_ , QueryTree::ORDER);
        BOOST_CHECK_EQUAL(queryTree->keyword_ , "대한민국 명바기");
        BOOST_CHECK_EQUAL(queryTree->children_.size() , 2U );
        childIter = queryTree->children_.begin();
        keywordTreeCheck(*childIter++, "대한민국");
        keywordTreeCheck(*childIter, "명바기");

        // Check with language analysis
        BOOST_CHECK(queryParser.getAnalyzedQueryTree(true, analysisInfo, queryUStr, queryTree, false, true));
        BOOST_CHECK_EQUAL(queryTree->type_ , QueryTree::ORDER);
        BOOST_CHECK_EQUAL(queryTree->keyword_ , "대한민국 명바기");
        BOOST_CHECK_EQUAL(queryTree->children_.size() , 3U );
        childIter = queryTree->children_.begin();
        keywordTreeCheck(*childIter++, "대한");
        keywordTreeCheck(*childIter++, "민국");
        keywordTreeCheck(*childIter++, "명바");
    }
    { // Nearby Type
        izenelib::util::UString queryUStr("{대한민국 명바기}^777", izenelib::util::UString::UTF_8);
        QueryTreePtr queryTree;
        QTIter childIter;

        AnalysisInfo analysisInfo;
        analysisInfo.analyzerId_ = "la_korall";

        // Check without analysis
        BOOST_CHECK( queryParser.parseQuery(queryUStr, queryTree, false, true) );
        BOOST_CHECK_EQUAL(queryTree->type_ , QueryTree::NEARBY);
        BOOST_CHECK_EQUAL(queryTree->keyword_ , "대한민국 명바기");
        BOOST_CHECK_EQUAL(queryTree->distance_ , 777 );
        BOOST_CHECK_EQUAL(queryTree->children_.size() , 2U );
        childIter = queryTree->children_.begin();
        keywordTreeCheck(*childIter++, "대한민국");
        keywordTreeCheck(*childIter, "명바기");

        // Check with language analysis
        BOOST_CHECK(queryParser.getAnalyzedQueryTree(true, analysisInfo, queryUStr, queryTree, false, true));
        BOOST_CHECK_EQUAL(queryTree->type_ , QueryTree::NEARBY);
        BOOST_CHECK_EQUAL(queryTree->keyword_ , "대한민국 명바기");
        BOOST_CHECK_EQUAL(queryTree->distance_ , 777 );
        BOOST_CHECK_EQUAL(queryTree->children_.size() , 3U );
        childIter = queryTree->children_.begin();
        keywordTreeCheck(*childIter++, "대한");
        keywordTreeCheck(*childIter++, "민국");
        keywordTreeCheck(*childIter++, "명바");
    }
}

BOOST_AUTO_TEST_SUITE_END()
