#include <idmlib/duplicate-detection/psm.h>
#include <util/ustring/UString.h>
#include <boost/lexical_cast.hpp>

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
using namespace sf1r;
using namespace std;
using namespace boost;

//BOOST_AUTO_TEST_SUITE(cdbffd)


//BOOST_AUTO_TEST_CASE(cdbInsert)
int main(int ac, char** av)
{
        string knowledge_dir;//="/home/lscm/codebase/b5m-operation/db/working_b5m/knowledge/";
        string cma_path;//="/home/lscm/codebase/icma/db/icwb/utf8";
        string scd_path;//="/home/lscm/6indexSCD/";
        if(ac==4)
        {
           //LOG(INFO)<<"the command should be like this:   ./b5m_product_matcher  knowledge_dir  cma_path  scd_path";
           knowledge_dir=av[1];
           cma_path=av[2];
           scd_path=av[3];
        }
        else
        {
           cout<<"the command should be like this:   ./b5m_product_matcher  knowledge_dir  cma_path  scd_path"<<endl;
           return 0;
        }
        bool noprice=false;
        int max_depth=3;
        if( knowledge_dir.empty())
        {
            return EXIT_FAILURE;
        }
        ProductMatcher matcher;
        matcher.SetCmaPath(cma_path);
        if(!matcher.Open(knowledge_dir))
        {
            LOG(ERROR)<<"matcher open failed"<<std::endl;
            return EXIT_FAILURE;
        }
        if(noprice)
        {
            matcher.SetUsePriceSim(false);
        }
        if(max_depth>0)
        {
            matcher.SetCategoryMaxDepth(max_depth);
        }
        if(!scd_path.empty())
        {
            //LOG(ERROR)<<"scd path empty"<<std::endl;
            vector<string>  txt;
            txt.push_back("限时包邮2012春装新款大摆半身裙波西米亚长裙夏季蓬蓬裙网纱裙");
            txt.push_back("秋冬新款欧美风厚实喇叭大裙摆黑灰裙裤打底裤显瘦裤裙");
            txt.push_back("韩国代购stylenanda左右不对称黑色长裙");
            txt.push_back("盘点世界上令人神往的“七大浪漫奇迹” - 港澳台 - 凤凰论坛 ");
            txt.push_back("七匹狼运动专卖店,七匹狼羽绒服 可卸帽高档保暖白鸭绒外套男 冬装特价运动男装,七匹狼,羽绒服,可卸帽,高档,保暖,鸭绒,外套,冬装,特价,运动,男装 ");
            txt.push_back("IBM E420 E420(1141A86) Thinkpad 新品促销 ");
            txt.push_back("大陆行货 Apple/苹果 IPHONE 4 (8G) iPhone4 全新正品 全国联保 ");
            txt.push_back("温哥华的艺术家peter pierobon的家具作品： “我的灵感主要来自世界美术，尤其是来自世界各地的土著文化。主要是木头制作而成，我的家具，旨在满足功能需求，具有挑战性的设计概念。” ");
            txt.push_back("重庆特产 周君记调料 红烧牛肉面佐...  ");
            txt.push_back("I教师节浪漫创意礼物韩国风格圆点爱心系列女生实用可爱纸巾套 ");
            txt.push_back("“魔鬼游泳池”号称世界上最危险的游泳池，位于赞比亚与津巴布韦边界的维多利亚瀑布，它地处110米高的维多利亚瀑布顶部，那些想挑战自己胆量极限的游客可以扒在“池边”，亲历奔腾的瀑布从身下飞流直下的刺激，不过最重要的是先要确定自己没有心脏病。酷旅图 http://www.coollvtu.com “魔鬼游泳池”号称世界上最危险的游泳池，位于赞比亚与津巴布韦边界的维多利亚瀑布，它地处110米高的维多利亚瀑布顶部，那些想挑战自己胆量极限的游客可以扒在“池边”，亲历奔腾的瀑布从身下飞流直下的刺激，不过最重要的是先要确定自己没有心脏病。酷旅图 http://www.coollvtu.com");


 
            for(unsigned i=0;i<txt.size();i++)
            {
               cout<<txt[i]<<"  ";
               std::vector<UString> results;
               UString text(txt[i], UString::UTF_8);
               matcher.GetFrontendCategory(text, 3, results);
               cout<<"result:"<<results.size();
               for(unsigned j=0;j<results.size();j++)
               {   string str;
                   results[j].convertString(str, UString::UTF_8);
                   cout<<str<<"  ";
               }
               cout<<endl;
            }
            //matcher.Test(scd_path);
        }
        else
        {
            matcher.SetUsePriceSim(false);
            while(true)
            {
                std::string line;
                std::cerr<<"input text:"<<std::endl;
                getline(std::cin, line);
                boost::algorithm::trim(line);
                Document doc;
                doc.property("Title") = UString(line, UString::UTF_8);
                uint32_t limit = 3;
                std::vector<ProductMatcher::Product> products;
                matcher.Process(doc, limit, products);
                for(uint32_t i=0;i<products.size();i++)
                {
                    std::cout<<"[CATEGORY]"<<products[i].scategory<<std::endl;
                }
            }
        }

}


//BOOST_AUTO_TEST_SUITE_END()

