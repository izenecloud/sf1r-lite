#include <idmlib/duplicate-detection/psm.h>
#include <util/ustring/UString.h>
#include <boost/lexical_cast.hpp>
#include "../TestResources.h"
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <log-manager/UserQuery.h>
#include <log-manager/LogAnalysis.h>
#include <b5m-manager/comment_db.h>
#include <b5m-manager/product_matcher.h>
#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include "TestResources.h"
using namespace sf1r;
using namespace std;
using namespace boost;

unsigned getCategoryError=0;
unsigned ProcessError=0;
string toString(UString us)
{
    string str;
    us.convertString(str, izenelib::util::UString::UTF_8);
    return str;
}
string getTitle(Document doc)
{
    UString utitle;
    doc.getProperty("Title", utitle);
    return toString(utitle);
}

string getSource(Document doc)
{

    UString usource;
    doc.getProperty("Source", usource);
    return toString(usource);
}

string get(Document doc,string prop)
{

    UString ustr;
    doc.getProperty(prop, ustr);
    return toString(ustr);
}
/*
string getId(Document doc,string prop)
{
    uint128_t id;
    doc.getProperty(prop, id);
    return boost::lexical_cast<int>(id);
}
*/
void show(Document doc)
{
    cout<<get(doc,"Source")<<" "<<doc.property("DOCID")<<" "<<doc.property("uuid")<<" "<<get(doc,"Price")<<"  "<<get(doc,"Title")<<"  "<<get(doc,"Category")<<" "<<get(doc,"Attribute")<<"  mobile "<<doc.property("mobile") <<endl;
}


void show(ProductMatcher::Product product)
{
//    cout<<"product"<<product.spid<<"title"<< product.stitle<<"attributes"<<product.GetAttributeValue() 
    cout<<"frontcategory"<<product.fcategory<<"scategory"<<product.scategory<<"price"<<product.price<<"brand"<<product.sbrand<<endl;
}

bool checkProduct(ProductMatcher::Product &product,string &fcategory,string &scategory,string &sbrand)
{
    if(product.scategory.find(scategory)==string::npos)return false;
    if(product.fcategory.find(fcategory)==string::npos)return false;
    if(product.sbrand!=sbrand)return false;
    return true;
}
bool checkProcesss(ProductMatcher &matcher,string spid,string soid,string source,string date,string price,string attribute,string title,string fcategory,string scategory,string sbrand)
{
    Document doc;
    doc.property("DATE") = izenelib::util::UString(date, izenelib::util::UString::UTF_8);
    doc.property("DOCID") =     izenelib::util::UString(spid,izenelib::util::UString::UTF_8);
    doc.property("Title") = izenelib::util::UString(title, izenelib::util::UString::UTF_8);
    doc.property("Price") = izenelib::util::UString(price,izenelib::util::UString::UTF_8);
    doc.property("Source") = izenelib::util::UString(source,izenelib::util::UString::UTF_8);
    doc.property("Attribute") = izenelib::util::UString(attribute,izenelib::util::UString::UTF_8);

    ProductMatcher::Product result_product;
    matcher.Process(doc,result_product);
    bool check=true;

    {
        bool ret=checkProduct(result_product,fcategory, scategory, sbrand);
        if(!ret)
        {
            show(doc);
            show(result_product);
            check=false;
        }
    }
    if(!check)
    {
        ProcessError++;
    }
    return check;
}


void ProcessVector(ProductMatcher* matcher,vector<Document>& docvec, vector<ProductMatcher::Product>& result_product)
{
    result_product.resize(docvec.size());
    for(unsigned i=0; i<docvec.size(); i++)
    {
        matcher->Process(docvec[i], result_product[i]);
    }
}


void CheckProcessVector(ProductMatcher* matcher,vector<Document>& doc,  vector<ProductMatcher::Product>& product)
{
    ProductMatcher::Product re;
    for(unsigned i=0; i<doc.size(); i++)
    {

        checkProcesss((*matcher),"", "",get(doc[i],"Source"), get(doc[i],"DATE"), get(doc[i],"Price"), get(doc[i],"Attribute"), get(doc[i],"Title"),product[i].fcategory,  product[i].scategory, product[i].sbrand);
    }
}



void MultiThreadProcessTest(string scd_file, std::string knowledge_dir)
{
    unsigned threadnum=4;
    std::string cma_path= IZENECMA_KNOWLEDGE ;
    //std::string knowledge_dir= MATCHER_KNOWLEDGE;
    string file=MOBILE_SOURCE;//"/home/lscm/codebase/b5m-operation/config/collection/mobile_source.txt";
    //std::vector<ProductMatcher*> matcher;
    ProductMatcher*  matcher=(new ProductMatcher);;
    matcher->SetCmaPath(cma_path);
    matcher->Open(knowledge_dir);

    std::vector<boost::thread*>  compute_threads_;
    compute_threads_.resize(threadnum);
    /*
    for(unsigned i=0; i<threadnum; i++)
    {
        matcher.push_back(new ProductMatcher);
        matcher[i]->SetCmaPath(cma_path);
        matcher[i]->Open(knowledge_dir);

    }
    */
    //boost::thread_group threads;
    vector<vector<Document> > threaddocvec;

    vector<Document> right;
    threaddocvec.resize(threadnum);

    ScdParser parser(izenelib::util::UString::UTF_8);
    parser.load(scd_file);
    uint32_t n=0;
    for( ScdParser::iterator doc_iter = parser.begin();
            doc_iter!= parser.end(); ++doc_iter, ++n)
    {

        if(n%10000==0)
        {
            LOG(INFO)<<"Find Documents "<<n<<std::endl;
        }
        if(n>40000)
        {
            break;
        }
        Document doc;
        SCDDoc& scddoc = *(*doc_iter);
        SCDDoc::iterator p = scddoc.begin();

        for(; p!=scddoc.end(); ++p)
        {
            const std::string& property_name = p->first;
            doc.property(property_name) = p->second;
        }

        threaddocvec[n%threadnum].push_back(doc);

    }
    vector<vector<ProductMatcher::Product> > Product;
    Product.resize(threadnum);
    //single thread to get right answer
    for(unsigned i=0; i<threadnum; i++)
    {
         ProcessVector(matcher,threaddocvec[i],Product[i]);

    }
    // multithread thread for check
    for(unsigned i=0; i<threadnum; i++)
    {
        compute_threads_[i]=(new boost::thread(boost::bind(&CheckProcessVector,matcher,threaddocvec[i],Product[i])));
        //compute_threads_[i]=(new boost::thread(boost::bind(&Process)));
    }
    cout<<"preadd"<<endl;
    for(unsigned i=0; i<threadnum; i++)
    {
        compute_threads_[i]->join();
    }
    //threads.join_all();

    

    
    
    delete matcher;


};

int main(int ac, char** av)
{

    cout<<"here!"<<endl;
    std::string cma_path= IZENECMA_KNOWLEDGE ;
    std::string knowledge_dir;//= MATCHER_KNOWLEDGE;
    string scd_file;//= TEST_SCD_PATH;

    if(ac==3)
    {
       //LOG(INFO)<<"the command should be like this:   ./b5m_product_matcher  knowledge_dir  cma_path  scd_path";
       knowledge_dir=av[1];
       scd_file=av[2];
    }
    else
    {
       cout<<"the command should be like this:   ./b5m_product_matcher  mathcer_knowledge_dir  scd_path(for multithread test)"<<endl;
       return 0;
    }
        /**/
    bool noprice=false;
    int max_depth=3;

    if( knowledge_dir.empty())
    {
    }
    ProductMatcher matcher;
    matcher.SetCmaPath(cma_path);
    if(!matcher.Open(knowledge_dir))
    {
        LOG(ERROR)<<"matcher open failed"<<std::endl;
    }
    if(noprice)
    {
        matcher.SetUsePriceSim(false);
    }
    if(max_depth>0)
    {
        matcher.SetCategoryMaxDepth(max_depth);
    }

    vector<std::pair<string,string> >  txt;
    txt.push_back(make_pair("限时包邮2012春装新款大摆半身裙波西米亚长裙夏季蓬蓬裙网纱裙","服饰鞋帽>女装>裙子"));
    txt.push_back(make_pair("秋冬新款欧美风厚实喇叭大裙摆黑灰裙裤打底裤显瘦裤裙","服饰鞋帽>女装>裙子"));
    txt.push_back(make_pair("韩国代购stylenanda左右不对称黑色长裙","服饰鞋帽>女装>裙子"));
    txt.push_back(make_pair("七匹狼运动专卖店,七匹狼羽绒服 可卸帽高档保暖白鸭绒外套男 冬装特价运动男装,七匹狼,羽绒服,可卸帽,高档,保暖,鸭绒,外套,冬装,特价,运动,男装 ","服饰鞋帽>男装>羽绒服"));
    txt.push_back(make_pair("IBM E420 E420(1141A86) Thinkpad 新品促销 ","电脑办公>电脑整机>笔记本"));
    txt.push_back(make_pair("大陆行货 Apple/苹果 IPHONE 4 (8G) iPhone4 全新正品 全国联保 ","手机数码>手机通讯>手机"));
    txt.push_back(make_pair("孝傲江湖 秋季新款女鞋高品质亮丽漆皮女时尚休闲女鞋蝴蝶结布鞋175222 黑色 35 ","服饰鞋帽>女鞋>布鞋"));
    txt.push_back(make_pair("LILY女士亮桔横机连衣裙111440B794913,L,12Q3","服饰鞋帽>女装>裙子"));
    txt.push_back(make_pair("LILY女士亮桔横机连衣裙111440B794913,L,12Q3","服饰鞋帽>女装>裙子"));
    txt.push_back(make_pair("Janeiredale珍爱芮德四合一矿物质奇幻粉饼（卡其色）","个护化妆>魅力彩妆>粉底/遮瑕"));
    txt.push_back(make_pair("ST&SAT星期六咖啡色皮带装饰牛皮圆头锥形跟拉链女士靴子SS14S9600721，39","服饰鞋帽>女鞋>靴子"));
    txt.push_back(make_pair("U'db花色女士精纺羊毛披肩YW1039，F","服饰鞋帽>配饰"));
    txt.push_back(make_pair("Max Mara麦丝玛拉黑色羊毛混纺腰带装饰优雅女士连衣裙，66260413，05，S","服饰鞋帽>女装>裙子"));
    txt.push_back(make_pair("蛋生 有机棉 棕白条纹长袖偏衫BNYU00L01C21100873","服饰鞋帽>童装>宝宝服饰"));
    txt.push_back(make_pair("BALENCIAGA巴黎世家玫瑰红羊皮材质经典款CLASSIC WEEKENDER女士手提机车包，110506 D94JT 5765","礼品箱包"));
    txt.push_back(make_pair("HappyLane幸福里漆牛皮名媛单肩包110703006","礼品箱包>潮流女包>单肩包"));
    txt.push_back(make_pair("Inidy爱尼德【自然】PT950铂金钻石吊坠,30分钻石吊坠/AD1000315","珠宝饰品"));
    txt.push_back(make_pair("MG美即左旋VC嫩白面膜25g*5p","个护化妆>面部护理>面膜"));
    txt.push_back(make_pair("BRIONI布莱奥尼暗蓝色纯棉简洁男士直筒休闲裤,PANTS PORTORICO2636 149121 00-PPT41X-54","服饰鞋帽>男装>裤子"));
    txt.push_back(make_pair("帕菲帕贝藏青男童百搭平织夹里长裤PDQS22P2156150,150,12Q3","服饰鞋帽>童装"));
    txt.push_back(make_pair("李宁男子时尚透气连帽开衫夹克AJDE010-1","服饰鞋帽>男装>外套"));
    txt.push_back(make_pair("予爱IA丶ai孕妇专用 有机茶树油清肌喷雾YA00197","个护化妆>面部护理>爽肤水"));
    txt.push_back(make_pair("童壹库 优雅公主插肩袖纯棉女童圆领长袖棉T恤KUWE091201柠檬黄，120,12Q3","服饰鞋帽>童装"));
    txt.push_back(make_pair("Mobi garden牧高笛男士夹棉两件套冲锋衣MB031028，橄榄绿，2XL","运动健康>户外鞋服>户外服装"));
    txt.push_back(make_pair("LIU JO大红色混合材质蕾丝装饰女童半身裙，9957B13948，K61130 T0299，RED，12M","服饰鞋帽>女装>裙子"));
    txt.push_back(make_pair("玛可曼可2012秋冬啡花高端女装女装外套","服饰鞋帽>女装"));
    txt.push_back(make_pair("VALENTINO华伦天奴方形表盘指针式女士石英镶钻手表，V36SBQ9109SS009","钟表手表"));
    txt.push_back(make_pair("棉花共和国女装三角裤2条装CTR51111229，黑白XL，12Q2","服饰鞋帽>内衣>内裤"));
    txt.push_back(make_pair("【珂兰钻石】 永恒经典 18K金30分钻石吊坠赠银链KLPW019753","珠宝饰品>时尚饰品>项链"));
    txt.push_back(make_pair("SK-II护肤精华露150ml(神仙水)","个护化妆>面部护理>精华露"));
    txt.push_back(make_pair("派克兰帝Paclantic男童梭织滑雪棉裤LPCE105502海军蓝160","服饰鞋帽>女装>裤子"));
    txt.push_back(make_pair("momento 全棉活性印花+绣花 水墨画 方枕 含芯","家用电器>健康电器"));
    txt.push_back(make_pair("12Q2ELLE蕾丝印花文胸1DB017紫色C80","服饰鞋帽>内衣>文胸"));
    txt.push_back(make_pair("MO&Co.摩安珂12Q3军绿英伦军装风单排扣外套M113COT75,L码","服饰鞋帽>女装>外套"));
    txt.push_back(make_pair("MISSONI米索尼男士领带，09-00D-002F","服饰鞋帽>配饰>领带"));
    txt.push_back(make_pair("Champcar马布里2012最新款红棕男子休闲鞋M11666F,44","服饰鞋帽>男鞋>休闲鞋"));
    txt.push_back(make_pair("Calvin Klein Jeans卡尔文·克莱恩藏青色纯棉水钻字母图案女士长袖T恤,T-SHIRT,CWP20Q,7A7 MIDNIGHT,L","服饰鞋帽>女装>T恤"));
    txt.push_back(make_pair("dodo赠品dodo小BB霜6g","个护化妆>魅力彩妆>粉底/遮瑕"));
    txt.push_back(make_pair("曼邦男士真皮编织牛皮手拿包手抓包MB6088-5-122黑","礼品箱包>潮流女包>手拿包"));
    txt.push_back(make_pair("Agalloch沉香木磨砂暗纹牛皮钱夹MRP12766灰色","家居家装>生活日用"));
    txt.push_back(make_pair("PIANEGONDA银色银质圆形吊坠装饰女士项链，12Y128CR，CA010946","珠宝饰品>时尚饰品>项链"));
    txt.push_back(make_pair("LOREAL欧莱雅爱莎润养顺滑润发乳,400ml","个护化妆>身体护理>护发"));
    txt.push_back(make_pair("高歌12Q2深蓝套头小衫G2112C11111A60313A，L","服饰鞋帽>女装>裙子"));
    txt.push_back(make_pair("BURBERRY巴宝莉黑色混合材质经典格纹领口女童长袖POLO衫,9957B14373 B15801 POLO Y21 5A","服饰鞋帽>女装"));
    txt.push_back(make_pair("GIVENCHY纪梵希多色牛皮材质纯色Obsedia金属标志女士单肩包，BAG 12H 5240 0","礼品箱包"));
    txt.push_back(make_pair("Inidy爱尼德天然玉石镀金镶嵌【鹊上枝头】S990足银纯银吊坠送红绳附国检证书-SL1000079","珠宝饰品>时尚饰品>项链"));
    txt.push_back(make_pair("THE NORTH FACE乐斯菲斯防风、保暖女款羽绒服,AAPR-JK3,黑色,L","服饰鞋帽>童装"));
    txt.push_back(make_pair("DIOR迪奥灰色桑蚕丝材质字母样式男士领带，12Y087MA 11E8880 800","服饰鞋帽>配饰>领带"));
    txt.push_back(make_pair("HR男背提包H12-201841A1,黑色","礼品箱包>潮流女包>手拿包"));
    txt.push_back(make_pair("鬼冢虎Onitsuka tiger休闲板鞋浅紫色D0B0N--0016，40.5号，12Q4",""));
    txt.push_back(make_pair("IPOD NANO 16GB BLUE MD477CH/A","手机数码>时尚影音>MP3/MP4"));
    txt.push_back(make_pair("Ferrari Earphones耳机T150i（黑色）","手机数码>时尚影音>耳机/耳麦"));
    txt.push_back(make_pair("饰典缤纷侧影围巾,S123A0687H0","服饰鞋帽>配饰>围巾"));
    txt.push_back(make_pair("易千家（LUCKY HOME）米兰幻想 四件套 1.8米","家居家装>家纺>床品件套"));
    txt.push_back(make_pair("TWICE棕色亮片点缀短针织手套TW00040","服饰鞋帽>配饰>围巾"));
    txt.push_back(make_pair("LILY女士桔红色梭织大衣112130C107943,L,12Q3","服饰鞋帽>女装>外套"));
    txt.push_back(make_pair("Jack Jones/杰克琼斯纯棉修身男士长袖针织衫B(黑)|212302002010,XXXL,1","服饰鞋帽>女装>针织"));
    txt.push_back(make_pair("ONLY低腰捏褶萝卜腿小脚口休闲裤LO(黑/Black|112314015010,XXLR,12Q3","服饰鞋帽>女装>裤子"));
    txt.push_back(make_pair("SPEEDO速比涛女装12''沙滩裤23220729，4号","运动健康>户外鞋服>户外服装"));
    txt.push_back(make_pair("ooh Dear-爱的立方 18K金吊坠（白） 0.64g~~0.68g 白色（独享0","珠宝饰品>时尚饰品>项链"));
    txt.push_back(make_pair("周大福/福星宝宝系列(瑶族服饰小版)/家和宝宝/黄金吊坠/R7487","珠宝饰品>时尚饰品>项链"));
    txt.push_back(make_pair("Adidas Neo阿迪达斯生活系列11年春季女子休闲鞋U45558,6","服饰鞋帽>运动>运动女鞋"));
    txt.push_back(make_pair("Jack Jones纯棉修身拼接男士衬衫B(深蓝)|212405008031,XXXL,12Q4","服饰鞋帽>女装>裙子"));
    txt.push_back(make_pair("NIKE耐克男子针织长裤519917-010,XL",""));
    txt.push_back(make_pair("MISUN米尚12Q3黑色OL商务压褶修身半裙 11411582/91/L，L","服饰鞋帽>女装>裙子"));
    txt.push_back(make_pair("Shuuemura植村秀臻皙净透美白水凝霜50ml","个护化妆>面部护理>面霜"));
    txt.push_back(make_pair("中音紫色试音王（黑胶CD）9787888575707，F","电脑办公>办公文仪>学生文具"));
    txt.push_back(make_pair("Mokymuky某客T恤男士MDT014，绿色，XXXL","服饰鞋帽>女装>T恤"));
    txt.push_back(make_pair("D&G杜嘉班纳黑色混合材质LOGO刺绣休闲男士短袖T恤,T-SHIRT,N80086_N0000,BLACK,M","服饰鞋帽>女装"));
    txt.push_back(make_pair("HERBORIST佰草集平衡洁面乳洗面奶100ml","个护化妆>面部护理>洁面乳"));
    txt.push_back(make_pair("建筑造型基础","个护化妆>身体护理>染发/造型"));
    txt.push_back(make_pair("探索.发现.生物小百科(全3册)","图书音像>音像>生活百科"));
    txt.push_back(make_pair("清仓15号现货童装韩国进口代购Clari女童雪纺长袖T恤[2A0772]0817","服饰鞋帽>童装"));
    txt.push_back(make_pair("永龙锡器黄金万两茶叶罐HJ-002","厨具>餐具>茶具/咖啡具"));
    txt.push_back(make_pair("真维斯男装新品纯棉圆领印花男士短袖T恤休闲t恤原价70","服饰鞋帽>男装>T恤"));
    txt.push_back(make_pair("elvandress官网正品 韩国直送 2013新款TS339 모던 투톤단가라 R T (화이트)","服饰鞋帽>女装>裙子"));
    txt.push_back(make_pair("韩风 flora 秋季保暖舒适外搭纯色开襟衫-ga-2513","服饰鞋帽>女装>外套"));
    txt.push_back(make_pair("LENS 朗斯淋浴房-利玛P42 玻璃8MM/10MM非标订做 （每平米价格）","家居家装>家具"));
    for(unsigned i=0; i<txt.size(); i++)
    {
        //cout<<txt[i]<<"  ";
        std::vector<UString> results;
        UString text(txt[i].first, UString::UTF_8);
        matcher.GetFrontendCategory(text, 3, results);
        //cout<<"result:"<<results.size();
        bool get=false;
        for(unsigned j=0; j<results.size(); j++)
        {
            string str;
            results[j].convertString(str, UString::UTF_8);
            //cout<<str;
            if(str==txt[i].second)
            {
                get=true;
            }
        }
        if(!get)
        {
            std::cout<<"error"<<txt[i].first<<"RightAnswer"<<txt[i].second<<"size"<<results.size();
            for(unsigned j=0; j<results.size(); j++)
            {
                string str;
                results[j].convertString(str, UString::UTF_8);
                cout<<str;
            }
            getCategoryError++;
            cout<<endl;
        }
        //cout<<endl;
    }
    cout<<"错误率："<<double(getCategoryError)/txt.size()<<endl;


    checkProcesss(matcher,"7bc999f5d10830d0c59487bd48a73cae","46c999f5d10830d0c59487bd48adce8a","苏宁","20130301","1043","产地:中国,质量:优","华硕  TF700T","电脑办公>电脑整机>笔记本","笔记本电脑","Asus/华硕");
    checkProcesss(matcher,"72c999f5d10830d0c59487bd48a73cae","35c999f5d10830d0c59487bd48adce8a","凡客","20130301","1043","产地:韩国,质量:差","2012秋冬新款女外精品棉大衣","服饰鞋帽>女装","女装/女士精品","");
    checkProcesss(matcher,"","4d33c599547b17615efa9384bbcf3851","银泰精品百货官网","20130105174125","299.0","","LILY女士亮桔横机连衣裙111440B794913,L,12Q3","服饰鞋帽>女装>裙子","女装/女士精品>连衣裙","");
    checkProcesss(matcher,"","68b05f930be812ac800b4893852dfef5","银泰精品百货官网","20130105174125","209.0","","迪艾宝粉红色布纹牛皮长款钱包DY11D31204PK0","礼品箱包","箱包皮具/热销女包/男包","");
    checkProcesss(matcher,"","02e35d81fe6fb9093915457372e068a0","银泰精品百货官网","20130104174024","165.0","","VICHY薇姿泉之净滢润泡沫洁面膏125ml(新旧包装交替)","个护化妆>面部护理>洁面乳","美容护肤/美体/精油>洁面","");
    checkProcesss(matcher,"","44227ffdbc3d8d9b3a362982fc0c8fce","银泰精品百货官网","20130104174024","2500.0","","BALLY巴利深褐色牛皮材质纯色单肩包，BAG BAMTOR6171841","礼品箱包>时尚男包>商务公文包","箱包皮具/热销女包/男包>男包>公文包","");
    checkProcesss(matcher,"","4292a9c1458d46196f83ea488520ebc8","银泰精品百货官网","20130104174024","360.0","","MINNETONKA棕色麂皮材质简洁流苏装饰女童休闲短靴,shoes,2292,size12","服饰鞋帽>女鞋>靴子","女鞋>靴子","");
    checkProcesss(matcher,"","ba3568f7063223a197d3aebda38ae1b4","银泰精品百货官网","20130101174056","4055.0","","TECHNO MARINE铁克龙圆形表盘三眼计时女士手表，109006","钟表手表","钟表首饰>钟表","");
    checkProcesss(matcher,"","c665aa83a630bd3901a18b3f011c0f86","银泰精品百货官网","20130101174056","100.0","","12Q3爱帝2012秋冬新款女式棉莫代尔基础圆领内衣套装6126110111孔雀兰XXL","服饰鞋帽>内衣","女装/女士精品>女士内衣","");
    checkProcesss(matcher,"","2eed2d510022e5a0d65fa6d8b4ec7264","银泰精品百货官网","20121230174006","4780.0","","周生生黄金(足金)玻璃CharmmyKitty锁匙衬珠吊坠55158p","珠宝饰品>时尚饰品>项链","钟表首饰>项链","");
    checkProcesss(matcher,"","5fba3ee0fcf6d2ee204b6444911847fa","银泰精品百货官网","20121112220025","176.0","","ADIDAS阿迪达斯I.D.3系列男子短袖Polo衫P73136","","运动服/运动包/颈环配件>运动套装","Adidas/阿迪达斯");
    checkProcesss(matcher,"","5ffe414c556b047a35e27279dab14886","银泰精品百货官网","20121122220017","86.0","","良良 精纺苎麻保健袜（0-1岁）六双装 12Q3","服饰鞋帽>童装>儿童配饰","童装/童鞋/亲子装>袜子","");
    checkProcesss(matcher,"","eae3673063af79292a1472b8d4076ced","银泰精品百货官网","20121203155713","1350.0","","BALENCIAGA巴黎世家白色纯棉时尚印花图案女士背心,TOP 294292 TBK99-1070-S","服饰鞋帽>女装>裙子","女装/女士精品>连衣裙","");
    checkProcesss(matcher,"","1b9b220b71d4e149cc2a41121ce1e3c2","银泰精品百货官网","20121203155713","143.0","","帕菲帕贝米白男童织有领上衣PDQZ02P1501160,160,12Q3","服饰鞋帽>男装>外套","男装>夹克","");
    checkProcesss(matcher,"","f91c30ed03c4e62080649ace84f10d35","银泰精品百货官网","20121203155713","164.0","","KAPPA卡帕春秋BASIC系列男士双肩包K0158BS72-566","礼品箱包","箱包皮具/热销女包/男包","");
    checkProcesss(matcher,"","05bb1d79f0e003fbb903bc55f16a4ac6","银泰精品百货官网","20121201155526","169.0","","予爱IA丶ai孕妇专用 眼部新生精华霜YA00623","母婴>妈妈专区","孕妇装/孕产妇用品/营养>妈妈产前产后用品","");
    checkProcesss(matcher,"","7e1e3bf53c0d0a5a678c1e75be5bda8a","银泰精品百货官网","20121122220017","499.0","","童壹库 都市学院拼接撞色男童羽绒服KJDE115101海蓝/红，130,12Q3","服饰鞋帽>童装","童装/童鞋/亲子装","");
    checkProcesss(matcher,"","228e17286c22d980d00ada664e5e812b","银泰精品百货官网","20121122220017","990.0","","NINEWEST玖熙黑色车缝线人造革女士手提/斜挎两用包0249305NW","礼品箱包>潮流女包","箱包皮具/热销女包/男包>女包","");
    checkProcesss(matcher,"","bb7729dedaa7da8c69e779eceb894c42","银泰精品百货官网","20121122220017","239.0","","歌莉娅12Q3女士粉底印花小翻领衬衫28E3D04B，L","服饰鞋帽>女装>衬衫","女装/女士精品>衬衫","");
    checkProcesss(matcher,"","9cb910bc478e20b9300d2298a7139ffd","银泰精品百货官网","20121223174002","166.0","","Gapon银色海岸马赛克玻璃花瓶0814T3905-0814T银白色21*21*33cm","家居家装>家装建材>园艺","鲜花速递/花卉仿真/绿植园艺>花瓶/花器/花盆/花架","");
    checkProcesss(matcher,"","8351554aa03403a5ef2cdd1e236b5d3b","银泰精品百货官网","20121219173952","1395.0","","迪桑娜镂空流行风格牛皮单肩女包81212336220006","礼品箱包>潮流女包>单肩包","箱包皮具/热销女包/男包>女包>单肩包","");
    checkProcesss(matcher,"","20cc7e234972b6ea25f227752bfe718f","银泰精品百货官网","20130107174102","1204.0","","ARMANI JEANS阿玛尼深灰色混合材质纯色简约男士外套,GIUBBOTTO,Q6B85QT,E2,M","礼品箱包","箱包皮具/热销女包/男包","");
    checkProcesss(matcher,"","eaf373c6f81afec71caf76a233ad05fe","银泰精品百货官网","20130106174101","1799.0","","Vero Moda新款连帽狐狸毛装饰棉衣N(卡其色)|312322009133,XXL,12Q3","服饰鞋帽>女装>外套","女装/女士精品>风衣","");
    checkProcesss(matcher,"","edfaa5b916a82e2c691032c9dedc5893","银泰精品百货官网","20130105174125","98.0","","Skinvitals维肌泉多肽颈肩膜3片*38ml","个护化妆>面部护理>面膜","美容护肤/美体/精油>面膜/面膜粉","Skinvitals/维肌泉");
    checkProcesss(matcher,"","811e3a2b6f2826c87dd11686455a0db7","银泰精品百货官网","20130105174125","187.0","","皇冠红色女士经典时尚手提、单肩、斜跨女包EC1774，F","礼品箱包>潮流女包>单肩包","箱包皮具/热销女包/男包>女包>单肩包","");
    checkProcesss(matcher,"","41708c2e1ecb7a5c3ff7db7fa1fc554a","银泰精品百货官网","20130105174125","158.0","","御灵珠宝 生肖羊样样如意和田玉青玉牌（吊坠） 01091100022","珠宝饰品>时尚饰品>项链","钟表首饰>项链","");
    checkProcesss(matcher,"","bd835f14e037d6978d76f246af53d0ed","银泰精品百货官网","20130105174125","85.0","","TWICE紫色毛球时装帽MW00235","服饰鞋帽>配饰","服饰配件>帽子","");
    checkProcesss(matcher,"","20b5293b808918d925221400ccc67a94","银泰精品百货官网","20130105174125","205.0","","Haagendess哈根德斯女士多卡位牛皮钱包HA-30065R红色","礼品箱包","箱包皮具/热销女包/男包","");
    checkProcesss(matcher,"","18c380a075b41e360a51db68cb9ec906","银泰精品百货官网","20121116220016","135.0","","加菲猫 Garfield女童针织短款时尚夹克GJWE085202","服饰鞋帽>男装>外套","男装>夹克","");
    checkProcesss(matcher,"","7a607d2ab3f16c31555f3b5521a746a8","银泰精品百货官网","20121122220017","1506.0","","Gloire古名18K金钻石手链10160109 U1251HP3207WW","珠宝饰品>时尚饰品>项链","钟表首饰>项链","");
    checkProcesss(matcher,"","bd27161f117be091b7894be6e9dc1a52","银泰精品百货官网","20121122220017","172.0","","名仕男士企身包WMY014003-02黑色","礼品箱包","箱包皮具/热销女包/男包","");
    checkProcesss(matcher,"","9a306efa392f75f33fdacc50fe058e03","银泰精品百货官网","20121122220017","1180.0","","MONT BLANC万宝龙全框架时尚女士太阳镜，MB317S 6012P","运动健康>户外装备","ZIPPO/瑞士军刀/眼镜>太阳眼镜","");
    checkProcesss(matcher,"","e94b4318fa22231b9ce8acf0ac8d247b","银泰精品百货官网","20121122220017","299.0","","雪蔻12Q3女士绿色糖果甜蜜棉衣1111108203,XL","服饰鞋帽>女装>外套","女装/女士精品>卫衣/绒衫","");
    checkProcesss(matcher,"","f81bbaacf00c5bdea54921d377f00dde","银泰精品百货官网","20121122220017","4097.0","","【珂兰钻石】车字o行链 千足金项链7.97g KLNW018732","珠宝饰品>时尚饰品>项链","钟表首饰>项链","");
    checkProcesss(matcher,"","460c30fc05c22365ca9e43d8e2e8a64e","银泰精品百货官网","20121122220017","855.0","","ARMANI JEANS灰色牛皮材质圆孔装饰女士腰带,P5101,2A","服饰鞋帽>配饰>腰带","服饰配件>腰带/皮带/腰链","");
    checkProcesss(matcher,"","7cea90d9c6a73b4cbaab7e71e10622ee","银泰精品百货官网","20121122220017","710.0","","天宝龙凤千足金Q版生肖挂件 老鼠","珠宝饰品","钟表首饰>摆件","");
    checkProcesss(matcher,"","6897d585762249ae90f9f9d814c2058b","银泰精品百货官网","20121122220017","606.0","","MONNALISA白色混合材质印花女童T恤，416602PC,62010099,14A","服饰鞋帽>童装>女童","童装/童鞋/亲子装>女童裙装","");
    checkProcesss(matcher,"","24836934f4a7dd465bda59d235b33ddb","银泰精品百货官网","20121122220017","329.0","","Janeiredale珍爱芮德日光金灿","个护化妆>魅力彩妆>粉底/遮瑕","彩妆/香水/美妆工具>粉饼","");
    checkProcesss(matcher,"","e25ff5f20a5bd81fa6ffacf49d70a88a","银泰精品百货官网","20121206155812","680.0","","BOSSERT波士威尔棕色镂空牛皮圆头方跟套脚男士商务鞋155912J，43","服饰鞋帽>男鞋>正装鞋","流行男鞋>皮鞋","");
    checkProcesss(matcher,"","4bea9f3a9521d979f7d10d5fd9fd7cdc","银泰精品百货官网","20121206155812","300.0","","12Q4猫人极衣双心烫钻圆领女套U031020纯黑XL","服饰鞋帽>童装","童装/童鞋/亲子装","");
    checkProcesss(matcher,"","cdc8625db33474030b81e8519f54f22a","银泰精品百货官网","20121122220017","4233.0","","科因沃奇Coinwatch 皇家系列 全自动机械男装腕表C109SBK","钟表手表","钟表首饰>钟表","");
    checkProcesss(matcher,"","52a773802a8015aa18bff65fd1185dff","银泰精品百货官网","20121203155713","464.0","","emelysweetie12Q4碳黑裤子5211406350，40码","服饰鞋帽>男装>裤子","男装>休闲裤","");
    checkProcesss(matcher,"","b5f05059686f9f8145adf95636894cf0","银泰精品百货官网","20121217173916","795.0","","伊华欧秀21AAF0507HS36黑色12Q3女装通勤淑女气质纯色修身显瘦中长款风衣外套，S","服饰鞋帽>女装>外套","女装/女士精品>风衣","");
    checkProcesss(matcher,"","65f4fa320625b66ac3003c105cb61b17","银泰精品百货官网","20121215173900","8900.0","","GUCCI古琦多色混合材质经典双Glogo提花包身女士手提包，Bag 296896 F6BMG 85","礼品箱包","箱包皮具/热销女包/男包","");
    checkProcesss(matcher,"","b57356da33616eb0526f555c5bad96b8","银泰精品百货官网","20121215173900","2959.0","","茱茱12Q4橘红羽绒服J22340，L","服饰鞋帽>童装","童装/童鞋/亲子装","");
    checkProcesss(matcher,"","59e92c6ee1d4952a340f9902bcda599a","银泰精品百货官网","20121203155713","22.0","","SAMA（莎玛）家居拖鞋孔雀绒卡哇依小猫咪刺绣","家居家装>生活日用>家装软饰","床上用品/布艺软饰>十字绣/刺绣","");
    checkProcesss(matcher,"","98f3404ad306297196fb361dce1424a1","银泰精品百货官网","20130110114002","1149.0","","Max&Co宝蓝色女士无袖上装","服饰鞋帽>女装>裙子","女装/女士精品>连衣裙","");
    checkProcesss(matcher,"","c9e80a1133042a19e2b0dcadd4de291a","银泰精品百货官网","20130110115353","108.0","","ModeMarie曼黛玛琏葡萄紫色女士中模杯覆丝棉透气定型3/4蕾丝文胸R964020-P51,C8","服饰鞋帽>内衣>文胸","女装/女士精品>女士内衣>文胸","");
    checkProcesss(matcher,"","a7829d6fbd6eeda23010dce60f72fe59","银泰精品百货官网","20130109174048","2250.0","","MICHAEL KORS MK迈克·科尔斯Grayson系列pvc材质经典logo印花包身女士手提包，BAGS 30T1MGYS3B","礼品箱包","箱包皮具/热销女包/男包","");
    checkProcesss(matcher,"","f1662d8050ae0c50da09e54d754d1dd2","银泰精品百货官网","20130109174048","478.0","","TOREAD探路者户外女式徒步鞋油泥色TFAA92047，36号，12Q4","运动健康>户外鞋服>户外鞋袜","户外/登山/野营/旅行用品>户外鞋袜>登山鞋/徒步鞋/越野跑鞋","");
    checkProcesss(matcher,"","1db919e5b690e8cd4b0f60b5e3dcc9bb","银泰精品百货官网","20130109174048","649.0","","MO&Co.摩安珂12Q4酒红机车款可拆卸帽羽绒服M113EIN06,L码","服饰鞋帽>童装","童装/童鞋/亲子装","");
    checkProcesss(matcher,"","1d62a9dcd034397c4c7f4f247f4b3052","银泰精品百货官网","20130109174048","160.0","","CITING 2012新款英伦秋装蓝色纯棉翻领长袖卫衣 CT021171 ，XXL号","服饰鞋帽>女装>T恤","女装/女士精品>T恤","");
    checkProcesss(matcher,"","ef1cc8beed10e58d4238d439d3d1f996","银泰精品百货官网","20130109174048","179.0","","博洋家纺被芯 馨暖四季被 200*230cm","家居家装>家纺>被子","床上用品/布艺软饰>床上用品>被子/蚕丝被/羽绒被/棉被","");
    checkProcesss(matcher,"","e655ad4010be9fa8cf1f54dd8e83138f","银泰精品百货官网","20130110085635","499.0","","SPEEDO速比涛女装连体泳衣21024554，38号","运动健康>户外鞋服>户外服装","运动/瑜伽/健身/球迷用品>游泳>泳衣","");
    checkProcesss(matcher,"","111f2476d618021ac41d4537e6285b1c","银泰精品百货官网","20130110063113","999.0","","Gloire古名18K金钻石吊坠10158997 U1241P643WW","珠宝饰品>时尚饰品>项链","钟表首饰>项坠/吊坠","");
    checkProcesss(matcher,"","12cf5ccf71de03cc866aa5eb25584e4f","银泰精品百货官网","20130108174043","1150.0","","PUCCI璞琪女士太阳镜，M.EP641S C.104 T.58","服饰鞋帽>配饰>眼镜","ZIPPO/瑞士军刀/眼镜>框架眼镜","");
    checkProcesss(matcher,"","53a5fe26d3b69a5949f85c2d2126e3e7","银泰精品百货官网","20121227174030","93.0","","博洋家纺 夏威夷印花阳光毯（粉紫）180*200cm W91016201251","家居家装>家纺>毛巾被/毯","床上用品/布艺软饰>休闲毯/毛毯/绒毯","");
    checkProcesss(matcher,"","52a647e4beac85be80a342788d17beaf","银泰精品百货官网","20121226174022","5290.0","","CHLOE克洛伊多色混合材质拼色皮带搭配女士短袖连衣裙,DRESS,10E 10ERO09-10E075 15A,38","服饰鞋帽>女装>裙子","女装/女士精品>连衣裙","");
    checkProcesss(matcher,"","791b7f91af8e8613ea7c2f1d44262d05","银泰精品百货官网","20121226174022","688.0","","CK卡尔文·克莱恩黑色皮革材质纯色女士腰带，CW2100，999,90","服饰鞋帽>配饰>腰带","服饰配件>腰带/皮带/腰链","");
    checkProcesss(matcher,"","9f6a402cdab7ca526feb4bb33e1e92e8","银泰精品百货官网","20121224174027","414.0","","菲姿12Q4女式炭灰色羊毛衫FL831，L","服饰鞋帽>女装>裙子","女装/女士精品>连衣裙","");
    checkProcesss(matcher,"","15f301f5fc37eaef3981d803c82c2cb1","银泰精品百货官网","20121223174002","1298.0","","天威龙 DEVLON 金龙运宝镂空男装腕表 G2350间金","钟表手表","钟表首饰>钟表","");
    checkProcesss(matcher,"","86715d2bef792cd0c559ded56efdfbb5","银泰精品百货官网","20121122220017","520.0","","Lancome兰蔻柔皙轻透防晒乳SPF30+/PA+++,30ml","个护化妆>面部护理>防晒隔离","美容护肤/美体/精油>防晒","Lancome/兰蔻");
    checkProcesss(matcher,"","7dbcf1440d6976ddffc1d17377a7df15","银泰精品百货官网","20121217173916","69.0","","Septwolves七匹狼黑灰男士官方直营加绒加厚保暖背心02118,XXXL","服饰鞋帽>童装","童装/童鞋/亲子装","");
    checkProcesss(matcher,"","f4e0c879b7bb6c1cbb18cf5b64cf3d43","银泰精品百货官网","20121122220017","990.0","","CELINE赛琳半框架女士时尚眼镜VC1408M，0SBN，53","运动健康>户外装备","ZIPPO/瑞士军刀/眼镜>太阳眼镜","");
    checkProcesss(matcher,"","fc10c4f9837f9da0fe5be1d935d3f25b","银泰精品百货官网","20121201155526","999.0","","纳帕佳黑色连帽双排扣系腰带上衣L304111，M码，12Q3","服饰鞋帽>女装>裙子","女装/女士精品>连衣裙","");
    checkProcesss(matcher,"","e8186dc2fa5541c27c3a90c1e7516127","天猫","20130108210420","924.0","","周生生黄金(足金)风车花套装吊坠项坠 78718P(计价)","珠宝饰品>纯金K金饰品","珠宝/钻石/翡翠/黄金/奢侈品>黄金K金","");
    checkProcesss(matcher,"","40103b827688d5206aa7172450162be3","文轩网官网","20130108054822","13.1","","假如爱有天意","珠宝饰品>时尚饰品>戒指","钟表首饰>戒指/指环","");
    checkProcesss(matcher,"","ba56ee937fd019164aa7d8099f92dbe9","文轩网官网","20130108054822","24.0","","网络工程原理与实践教程","电脑办公>电脑配件","电脑硬件/显示器/电脑周边>网络/高清播放器","");

    checkProcesss(matcher,"","3dafbc5ccf96c00bb0185ef9c4480574","新世界百货网上商城官网","20130108191452","429.0","","多彩夏季羽绒被200*230cm 果绿","家居家装>家纺","床上用品/布艺软饰>床上用品","");
    checkProcesss(matcher,"","a8234a4573dd816f3126ffb42fa0eb32","京东商城","20130108202648","7.8","","茶左茶右黄山毛峰 2012年新茶叶 雨前新 二级清香黄山毛峰 绿茶 3g/袋","食品饮料>酒饮冲调>茗茶","茶/酒/冲饮>茶叶>绿茶","");
    checkProcesss(matcher,"","33cfe56bd5180d9f82170c6a64f95dc2","天猫","20130108204608","179.0","","[夜市疯抢]美特斯邦威男贴袋亮面潮流羽绒服外套238187原价499","服饰鞋帽>男装>羽绒服","男装>羽绒服","");
    checkProcesss(matcher,"","6606c65e41cf6d751d817380f3303e33","elvandress官网","20130108101651","139.31","","elvandress官网正品 韩国直送 2013新款ACC142 프리미엄 퀄리티 머플러 (5color)","服饰鞋帽>配饰>眼镜","ZIPPO/瑞士军刀/眼镜>眼镜架","");
    checkProcesss(matcher,"","1fe68c5ee325ccae1e12ec5b17716148","易迅网","20130108193733","69.0","","Langsha 浪莎 男士羊毛竹炭保暖内衣 深麻灰（1套装）185/110产品编号：28-267-10380","服饰鞋帽>内衣>保暖","女装/女士精品>女士内衣>保暖套装","");
    checkProcesss(matcher,"","a76d4a0ee3e087dd637f835060b25473","京东商城","20130108201942","56.0","","洁丽雅Grace 纯棉强吸水舒适浴巾 清爽柔肤纯棉浴巾 儿童浴巾 红色 浴巾","家居家装>家纺>毛巾家纺","床上用品/布艺软饰>毛巾/浴巾/浴袍>毛巾/面巾","");
    cout<<"错误率："<<double(ProcessError)/82<<endl;
    MultiThreadProcessTest(scd_file,knowledge_dir);
    cout<<"错误率："<<double(ProcessError)/40000<<endl;
    return 1;
};


/*

BOOST_AUTO_TEST_CASE(Multithread_case)
{
    unsigned threadnum=4;
    std::string cma_path= IZENECMA_KNOWLEDGE ;
    std::string knowledge_dir= MATCHER_KNOWLEDGE;
    string file=MOBILE_SOURCE;//"/home/lscm/codebase/b5m-operation/config/collection/mobile_source.txt";
    std::vector<ProductMatcher*> matcher;
    std::vector<boost::thread*>  compute_threads_;
    compute_threads_.resize(threadnum);
    for(unsigned i=0; i<threadnum; i++)
    {
        matcher.push_back(new ProductMatcher);
        matcher[i]->SetCmaPath(cma_path);
        bool ret=matcher[i]->Open(knowledge_dir);
        BOOST_CHECK_EQUAL(ret,true);
    }
    //boost::thread_group threads;
    vector<vector<Document> > threaddocvec;
    threaddocvec.resize(threadnum);
    string scd_file= TEST_SCD_PATH;
    ScdParser parser(izenelib::util::UString::UTF_8);
    parser.load(scd_file);
    uint32_t n=0;
    for( ScdParser::iterator doc_iter = parser.begin();
            doc_iter!= parser.end(); ++doc_iter, ++n)
    {

        if(n%10000==0)
        {
            LOG(INFO)<<"Find Documents "<<n<<std::endl;
        }
        if(n>40000)
        {
            break;
        }
        Document doc;
        SCDDoc& scddoc = *(*doc_iter);
        SCDDoc::iterator p = scddoc.begin();

        for(; p!=scddoc.end(); ++p)
        {
            const std::string& property_name = p->first;
            doc.property(property_name) = p->second;
        }

        threaddocvec[n%threadnum].push_back(doc);

    }

    for(unsigned i=0; i<threadnum; i++)
    {
        compute_threads_[i]=(new boost::thread(boost::bind(&Proce)));
    }
    cout<<"preadd"<<endl;
    for(unsigned i=0; i<threadnum; i++)
    {
        compute_threads_[i]->join();
    }
    //threads.join_all();
    for(unsigned i=0; i<threadnum; i++)
    {
        delete matcher[i];
    }

}
*/


