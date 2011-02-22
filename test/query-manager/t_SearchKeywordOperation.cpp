/*
///
/// @file   t_SearchKeywordOperation.cpp
/// @author Dohyun Yun
/// @date   2010-06-24
///

#include "test_def.h"

#include <boost/test/unit_test.hpp>
#include <query-manager/SearchKeywordOperation.h>

using namespace std;
using namespace sf1r;
using namespace boost::unit_test;
using izenelib::ir::idmanager::IDManager;
using izenelib::util::UString;

BOOST_AUTO_TEST_SUITE(QueryParser_test)

AnalysisInfo analysisInfo;
CollectionMeta collectionMeta;
boost::shared_ptr<LAManager> laManager;
boost::shared_ptr<IDManager> idManager;

void initOnlyOnce()
{
    ANALYZER analyzer = KOREAN_ALL;
    TOKENIZER tokenizer = NONE;
    LAManagerConfig config;
    analysisInfo = initConfig( analyzer, tokenizer, config );

    LAPool::getInstance()->init(config);
    laManager.reset( new LAManager() );
    ::system("rm -rf ./idm_qmt*");
    idManager.reset( new IDManager("./idm_qmt") );

    QueryParser::initOnlyOnce( laManager, idManager );

} // end - initOnlyOnce

BOOST_AUTO_TEST_CASE(normalizeQuery_test)
{
    initOnlyOnce();

    std::vector<std::string> queryStringList, normResultList;

    queryStringList.push_back("(  hello  |   world  )");
    queryStringList.push_back("{  test  case } ^ 12(month case)");
    queryStringList.push_back("(bracket close)(open space)");
    normResultList.push_back(std::string("(hello|world)"));
    normResultList.push_back(std::string("{test case}^12 (month case)"));
    normResultList.push_back(std::string("(bracket close) (open space)"));
    for(size_t i = 0; i < queryStringList.size(); i++)
    {
        std::string out;
        QueryParser::normalizeQuery( queryStringList[i], out );
        BOOST_CHECK_EQUAL( out , normResultList[i] );
    }
}

BOOST_AUTO_TEST_CASE(parseQuery_basic_test)
{
    std::string queryStr;
    UString queryUStr;
    QueryTreePtr queryTree;

    { // KEYWORD type
        queryStr = "본케나오게하지마라.";
        queryUStr.assign(queryStr, UString::UTF_8);
        BOOST_CHECK( QueryParser::parseQuery(queryUStr, queryTree, false) );
        keywordTreeCheck(queryTree, queryStr);
    }
    { // WILDCARD type 1
        queryStr = "본케나오게*하지마라?";
        queryUStr.assign(queryStr, UString::UTF_8);
        BOOST_CHECK( QueryParser::parseQuery(queryUStr, queryTree, false) );
        BOOST_CHECK_EQUAL( QueryTree::WILDCARD, queryTree->type_ );
        BOOST_CHECK_EQUAL( queryStr , queryTree->keyword_ );
        BOOST_CHECK_NE( 0 , queryTree->keywordId_ );
        BOOST_CHECK( queryUStr == queryTree->keywordUString_ );
        BOOST_CHECK_EQUAL( 0 , queryTree->children_.size() );
    }
    { // WILDCARD type 2
        queryStr = "본케나오게*하지마라?";
        queryUStr.assign(queryStr, UString::UTF_8);
        BOOST_CHECK( QueryParser::parseQuery(queryUStr, queryTree, true) );
        BOOST_CHECK_EQUAL( QueryTree::WILDCARD, queryTree->type_ );
        BOOST_CHECK_EQUAL( queryStr , queryTree->keyword_ );
        BOOST_CHECK( queryUStr == queryTree->keywordUString_ );
        BOOST_CHECK_NE( 0 , queryTree->keywordId_ );
        BOOST_CHECK_EQUAL( 11 , queryTree->children_.size() );
        std::list<QueryTreePtr>::const_iterator iter = queryTree->children_.begin();

        BOOST_CHECK_EQUAL( QueryTree::KEYWORD, (*iter)->type_ );
        BOOST_CHECK_EQUAL( "본" , (*iter)->keyword_ );
        BOOST_CHECK( UString("본", UString::UTF_8) ==(*iter)->keywordUString_ );
        BOOST_CHECK_NE( 0, (*iter)->keywordId_ );

        iter++;iter++;iter++;iter++;iter++;
        BOOST_CHECK_EQUAL( QueryTree::ASTERISK, (*iter)->type_ );

        iter++;iter++;
        BOOST_CHECK_EQUAL( QueryTree::KEYWORD, (*iter)->type_ );
        BOOST_CHECK_EQUAL( "지" , (*iter)->keyword_ );
        BOOST_CHECK( UString("지", UString::UTF_8) == (*iter)->keywordUString_ );
        BOOST_CHECK_NE( 0, (*iter)->keywordId_ );

        iter++;iter++;iter++;
        BOOST_CHECK_EQUAL( QueryTree::QUESTION_MARK, (*iter)->type_ );
    }
    { // AND type
        queryStr = "본케나오게 하지마라";
        queryUStr.assign(queryStr, UString::UTF_8);
        BOOST_CHECK( QueryParser::parseQuery(queryUStr, queryTree, true) );
        BOOST_CHECK_EQUAL( QueryTree::AND, queryTree->type_ );
        BOOST_CHECK_EQUAL( "", queryTree->keyword_ );
        BOOST_CHECK_EQUAL( 0 , queryTree->keywordId_ );
        BOOST_CHECK_EQUAL( 2 , queryTree->children_.size() );

        std::list<QueryTreePtr>::const_iterator iter = queryTree->children_.begin();
        QueryTreePtr child = *iter;
        keywordTreeCheck(child, "본케나오게");

        child = *(++iter);
        keywordTreeCheck(child, "하지마라");
    }
    { // OR type
        queryStr = "본케나오게|하지마라";
        queryUStr.assign(queryStr, UString::UTF_8);
        BOOST_CHECK( QueryParser::parseQuery(queryUStr, queryTree, true) );
        BOOST_CHECK_EQUAL( QueryTree::OR, queryTree->type_ );
        BOOST_CHECK_EQUAL( "", queryTree->keyword_ );
        BOOST_CHECK_EQUAL( 0 , queryTree->keywordId_ );
        BOOST_CHECK_EQUAL( 2 , queryTree->children_.size() );

        std::list<QueryTreePtr>::const_iterator iter = queryTree->children_.begin();
        QueryTreePtr child = *iter;
        keywordTreeCheck(child, "본케나오게");

        child = *(++iter);
        keywordTreeCheck(child, "하지마라");
    }
    { // NOT type
        queryStr = "!빙신";
        queryUStr.assign(queryStr, UString::UTF_8);
        BOOST_CHECK( QueryParser::parseQuery(queryUStr, queryTree, true) );
        BOOST_CHECK_EQUAL( QueryTree::NOT, queryTree->type_ );
        BOOST_CHECK_EQUAL( "", queryTree->keyword_ );
        BOOST_CHECK_EQUAL( 0 , queryTree->keywordId_ );
        BOOST_CHECK_EQUAL( 1 , queryTree->children_.size() );

        QueryTreePtr child = *(queryTree->children_.begin());
        keywordTreeCheck(child, "빙신");
    }
    { // EXACT type
        queryStr = "\"본케나오게 하지마라\"";
        queryUStr.assign(queryStr, UString::UTF_8);
        BOOST_CHECK( QueryParser::parseQuery(queryUStr, queryTree, true) );
        BOOST_CHECK_EQUAL( QueryTree::EXACT, queryTree->type_ );
        BOOST_CHECK_EQUAL( "본케나오게 하지마라", queryTree->keyword_ );
        BOOST_CHECK_NE( 0 , queryTree->keywordId_ );
        BOOST_CHECK_EQUAL( 2 , queryTree->children_.size() );

        std::list<QueryTreePtr>::const_iterator iter = queryTree->children_.begin();
        QueryTreePtr child = *iter;
        keywordTreeCheck(child, "본케나오게");

        child = *(++iter);
        keywordTreeCheck(child, "하지마라");
    }
    { // ORDER type
        queryStr = "[본케나오게 하지마라]";
        queryUStr.assign(queryStr, UString::UTF_8);
        BOOST_CHECK( QueryParser::parseQuery(queryUStr, queryTree, true) );
        BOOST_CHECK_EQUAL( QueryTree::ORDER, queryTree->type_ );
        BOOST_CHECK_EQUAL( "본케나오게 하지마라" , queryTree->keyword_ );
        BOOST_CHECK_NE( 0 , queryTree->keywordId_ );
        BOOST_CHECK_EQUAL( 2 , queryTree->children_.size() );

        std::list<QueryTreePtr>::const_iterator iter = queryTree->children_.begin();
        QueryTreePtr child = *iter;
        keywordTreeCheck(child, "본케나오게");

        child = *(++iter);
        keywordTreeCheck(child, "하지마라");
    }
    { // NEARBY type
        queryStr = "{본케나오게 하지마라}^1982";
        queryUStr.assign(queryStr, UString::UTF_8);
        BOOST_CHECK( QueryParser::parseQuery(queryUStr, queryTree, true) );
        BOOST_CHECK_EQUAL( QueryTree::NEARBY, queryTree->type_ );
        BOOST_CHECK_EQUAL( "본케나오게 하지마라" , queryTree->keyword_ );
        BOOST_CHECK_NE( 0 , queryTree->keywordId_ );
        BOOST_CHECK_EQUAL( 2 , queryTree->children_.size() );

        std::list<QueryTreePtr>::const_iterator iter = queryTree->children_.begin();
        QueryTreePtr child = *iter;
        keywordTreeCheck(child, "본케나오게");

        child = *(++iter);
        keywordTreeCheck(child, "하지마라");

        BOOST_CHECK_EQUAL( 1982 , queryTree->distance_ );
    }
}

BOOST_AUTO_TEST_CASE(parseQuery_extreme_test1)
{
    // Insert Large amount of query.
    string queryStr("[ 회사명.대표.자본금(단위 백만원).업종.주소 順 ]<>섬유*꾸메(이종배.50.의류) 역삼동 689의4*나무트레이딩 (이상녀.100.의류) 방학동 621*뉴월드콜렉션(나태현.70.의류) 하계동 250의3하계테크노타운에이-502*데코레이인터내셔날(최능기.50.의류) 신당동 122의4*디엠제이통상(김대인.50.의류) 신당동 155의1곡지빌딩 412호*미라스인터내셔날(최웅수.300.의류제조) 역삼동 659의4*씨루트글로벌(배흥용.50.의류제조) 독산동 302의6 2층*아리타워(백승구.50.의류) 충정로3가222*아미로이인터내셔날(하영숙.50.의류) 신당동 102의2*앨리슨패션(박철.50.고기능성섬유) 광희동 1가145의2 205호이조은침대(김용태.100.침구류) 용강동 51의5 영우빌딩 5층<>식품*광선축산(김광선.50.축산물) 마장동 510의3삼성빌딩 특7호*상명푸드(이상기.100.식품류) 방이동 51의4효성올림픽카운티1011호*새롬후디스(현성규.50.인삼제품) 서초동 1628의29*씨앤제이브라더스(안승길.50.식품류) 가락동 119의12 201호*엠디피생활건강(정인숙.500.건강식품) 문래동 3가55의7에이스테크노타워402호*해인바다(신상우.50.농수축임산물) 방이동 196의19<>유통*가나우드코리아(강상규.50.수출입) 영등포동 2가207*가백무역(최명자.50.의류) 신천동 11의9잠실한신코아오피스텔*건훈전자(황상하.300.전자부품) 서초동 1671의5대림서초리시온상가510호*경남아이앤티(이경남.300.통신기기) 역삼동 725의25포커스빌딩 2층*경한철강(김종민.50.철강) 문래동 3가54의43*그레비컴(노경선.50.신용카드조회기) 하계동 288현대2차(아) 상가지하1층전관*글로벌콘티넨탈코리아(박상호.100.재활용품자재) 역촌동 43의64*까사아이엔티(이한우.50.원단수출입) 포이동 240일영빌딩 2층*나노윈(최병영.50.정밀측정시스템) 가락동 171의11예울빌딩 3층*네트서브웨이(오정석.20.전산소모품) 역삼동 707의38테헤란오피스빌딩 1203호*다인인터컴(백광조.30.통신장비) 여의도동 11의11한서오피스텔512*다인티앤지(이광석.50.섬유) 염창동 264의26 2층*대솔아이티(최성민.50.컴퓨터주변기기) 정릉동 921의8*동 일엠디(강권.50.공산품) 양재동 23이비지니스타워504*두리농산(김광태.100.농산물) 동 작동 105*디엘가스산업(권석범.50.가스기기류) 공항동 1351*디티유어패럴(김미정.50.의류) 신당동 193의3 713호*라이프써핑(이종철.50.전자상거래) 서교동 395의166서교빌딩 602호*레몬통상(임종말.50.농수축산물) 가락동 98의7거북이빌딩 201호*레이스조이(천?牙?50.전자상거래) 양재동 20의12 112호*머리나라(김재남.770.무역) 석촌동 2의9금보빌딩 5층*메이크업카(김기엽.100.자동 차용품) 장안동 307의6대양빌딩 2층*명금골드(박희호.100.금은귀금속) 봉익동 168금정빌딩 304호*미건하우징(권영수.50.합성수지) 장위1동 219의482*범진티엔비(최수열.50.농수축산물) 방화동 621의15 2층*베리굿체인(김치석.50.식품) 안국동 175의87안국빌딩*뷰맥스코스매틱(정규혁.50.화장품) 방배동 456의1 206호*브랜드 (정연호.50.신발) 송파동 84조원빌딩 1층*비마촌(조태월.50.건강보조식품) 양재동 2의27대명빌딩 2층*비이포코리아(방상길.50.소프트웨어) 대치동 942의10해성2빌딩 3층*산해금속(박이석.50.금속) 구로동 604의1*상성무역(김상현.100.지금) 와룡동 38의1동 주빌딩 203호*새아로(신일진.50.의류) 신월동 974의12 2층*샛별농산(장길영.50.농산물) 가락동 600채소시장1층142의1호*서경쥬얼리(김소나.50.귀금속) 낙원동 90대성빌딩 602호*서림도서유통(박윤원.50.도서납품) 충정로3가72의6*석포개발(최정운.50.잡화) 구수동 60*세진에스제이(박준혁.50.귀금속) 한남동 635의1신화빌딩*셀레코(이인호.50.잡화) 청담동 93의5*수납건재(송옥금.200.가설재) 신정2동 296의10 2층*수정식품(김영길.50.농축수산물) 마장동 481의51*시지테크(김종기.50.컴퓨터주변기기) 당산동 2가164현대(아) 상가제비209호*시티엔(김재웅.100.홈네트워킹) 상계동 138의86*신우필라띠(송지철.50.원사) 장안4동 296의13*썬픽쳐스(유주성.50.영화) 포이동 229의9코인빌딩 4층*쓰리온(정봉채.50.식품잡화류) 영등포동 4가153의1덕수빌딩 431호*씨엔루나(이재화.100.화장품) 가락동 10의7대명빌딩*아사이실리카코리아(한훈.50.비철금속) 논현동 6의21세양에이팩스708호*아이피링크(신진숙.50.가전제품) 서교동 342의12오즈빌딩 6층*아인즈(김태동 .50.제조) 암사2동 514의65 2층*아피크(추병천.50.피부미용) 한남2동 737의28현대안성타워3층*에스엠비앤비(박상모.50.주류) 문정동 64의4동 호빌딩 신관302호*에이디에스엔코리아(이호영.50.기계) 도곡동 467의6대림아크로텔26층2619호*에이치앤드피케미팜(홍용상.210.의약품원료) 응암2동 443의26*엑스리전(정인철.50.전자상거래) 신천동 11의9한신잠실코아오피스텔1009호*엔티스시스템(조계윤.50.신용카드조회기) 자양동 598의37 1층*연덕이엔지(최덕룡(중국) .50.잡화) 구로5동 110의4도림베어스타워712호*예티스(오정훈.50.컴퓨터) 서교동 394의14명성빌딩 5층*오에이비즈(박경원.150.컴퓨터) 고척동 75의1 123전자타운1동 332호*오엠플러스(김영문.50.지금) 종로3가175의4현대빌딩 1101호*용명약품(김하용.550.의약품) 제기동 1158의32*우리피씨(김양태.50.컴퓨터) 한강로3가40의969관광터미널2층203호*우성아이티(나은주.50.문구) 등촌동 678의7동 광빌딩 402호*위고스포츠(박종익.50.스포츠용품) 논현동 180의15 2층*은호건업(이은주.100.건축자재) 상도동 204의50*이피맥스코리아(문당식.50.화장품) 서초동 1660의7*인덕종합상사(조현성.50.정보통신기기) 마천2동 49의12 201호*인스트랙(양근도.50.스포츠용품) 봉천동 1629의1덕원빌딩 4층*일진청과(김인택.50.농산물) 가락동 600한국청과내*재다물산(김종임.50.황옥정옥) 적선동 156플래티늄614*정명전자(이훈민.50.무역) 원효로3가51의30*정앤박컬렉션(허태숙.100.가구) 논현동 125의4*제이씨금은(최정근.50.지금) 묘동 203의1 502호*제이지인터내셔날(박기주.50.섬유수출입) 충정로2가191유원골든타워13층*조이앤조이네트워크(박장곤.50.제조) 역삼동 735의11신일유토빌906호*좋은세상만들기(고영걸.50.가정용기구) 양재동 4의3대흥빌딩 2층*중도파인로드(조수현.50.무역) 청량리동 712*지오젠(김중서.50.의약품) 서초동 1580의10*진성엠디엠(이충재.50.건축자재) 서초동 1357의6동 암빌딩*진현정보프라자(문상선.50.컴퓨터) 역삼동 837의18거성빌딩 비-312*차인씨앤씨(이수홍.50.컴퓨터) 한강로2가15의2나진상가17동 가열216호*차조아닷컴(서완석.50.자동 차) 등촌동 14의35*카지인터내셔날(카시프 시디크 카지(파키스탄).50.악세사리) 영등포동 6가 11영원빌딩 310호*케이에스엔케이(서현철.50.통신판매) 논현동 268의10*쿨픽스(김인태.200.전자제품) 남창동 1의1디-01호*크론티어(조원희.60.잡화) 성내동 397의1양지빌딩 703호*클리어링(이동 훈.100.전자상거래) 목동 923의14*테라디엔씨(정진용.50.컴퓨터) 잠원동 15의9*트러스틸(조신제.50.철강) 아현동 424의35*트윈푸드(김대열.50.우육) 마장동 818현대(아) 상가*파트너엔어씨스턴트(한혁.100.인터넷) 반포동 737의19온도빌딩*판인터내셔날(한종호.50.의류잡화) 방화동 849벽산에어트리움96호*포스켐(김기련.50.석유화학) 양재동 275의3*푸르미인터내셔널코퍼레이션(이현숙.50.난수출입) 공릉동 493의5 202호*피코에스앤비(전홍두.50.건자재) 미아동 1267의359*한국재난방재산업(손경보.500.소방관련기자재) 용두동 255의33*한영통운물류(한관섭.50.자동 차매매) 대림동 807의13덕산빌딩 2층*해피스마일(강호창.50.치과의료기) 양재동 324의1베델회관7층701*희망청과(조규재.50.농산물중개) 당산동 1가75<>인쇄.출판*가나칼라(이향국.100.인쇄) 필동 3가48의1*기브로또(박형초.300.인쇄업) 논현동 203신천빌딩 301호*더캔(최부헌.50.인쇄) 신사동 541의10*싸운즈굿(조면식.50.음반) 청담동 120의10신애빌딩 4층*쏘굿미디어에듀플랜(권세호.50.서적출판) 논현동 39의13*엔케이니드(설광수.50.홍보) 서초동 1359의35보일빌딩 602호*와이즈위(이흥무.100.도서출판) 대흥동 12의33캠프오피스텔902호*유빅스피앤텍(정미숙.50.복사용지) 마곡동 226의1*한월쉬핑가제트(국원경.100.잡지) 구로동 212의26이-스페이스빌딩 614호*홍암문화사(배홍수.50.출판물) 대림동 989의12한남빌딩 204호");
    UString queryUStr(queryStr, UString::UTF_8);
    QueryTreePtr queryTree;
    BOOST_CHECK( QueryParser::parseQuery(queryUStr, queryTree, false) );
}

BOOST_AUTO_TEST_CASE(getAnalyzedQueryTree_basic_test)
{
    {
        UString queryUStr("(reds|iphone4) (!빨간 자동차)", UString::UTF_8);
        QueryTreePtr queryTree;

        BOOST_CHECK(QueryParser::getAnalyzedQueryTree(true, analysisInfo, queryUStr, queryTree, false));

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
}

BOOST_AUTO_TEST_CASE(__finilize__)
{
    idManager->flush();
    LAPool::destroy();
}

BOOST_AUTO_TEST_SUITE_END()
    */
